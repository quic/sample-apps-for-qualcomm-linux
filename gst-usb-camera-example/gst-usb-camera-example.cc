/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Gstreamer Application:
 * Gstreamer Application for single USB Camera usecases with different possible
 * outputs
 *
 * Description:
 * This application Demonstrates single USB camera usecases with below possible
 * output:
 *     --Live Camera Preview on Display
 *     --Dump the Camera YUV to a file
 *
 * Usage:
 * Live Camera Preview on Display:
 * gst-usb-camera-example -o 0 -w 640 -h 480 -f 30 -F YUY2
 * For YUV dump on device:
 * gst-usb-camera-example -o 1 -w 640 -h 480 -f 30 -F YUY2
 *
 * Help:
 * gst-usb-camera-example --help
 *
 * *******************************************************************************
 * Live Camera Preview on Display:
 *     camerasrc->capsfilter->convert->waylandsink
 * Dump the Camera YUV to a filesink:
 *     camerasrc->capsfilter->convert->filesink
 * *******************************************************************************
 */

#include <gst/gst.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <glib-unix.h>

#define DEFAULT_OP_YUV_FILENAME "/opt/yuv_dump%d.yuv"
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_FRAMERATE 30
#define DEFAULT_FORMAT "YUY2"
#define MAX_VID_DEV_CNT 64

#define GST_APP_SUMMARY                                                       \
  "This app enables users to use a single USB camera for preview and YUV dump.\n" \
  "\nCommand:\n"                                                              \
  "For Preview on Display:\n"                                                 \
  "  gst_usb_camera_example -o 0 -w 640 -h 480 -f 30 -F YUY2\n"                    \
  "For YUV dump:\n"                                                           \
  "  gst_usb_camera_example -o 1 -w 640 -h 480 -f 30 -F YUY2\n"                    \
  "\nOutput:\n"                                                               \
  "  Upon execution, application will generates output as user selected. \n"  \
  "  In case of a preview, the output video will be displayed. \n"            \
  "  In case YUV dump the output video stored at /opt/yuv_dump%d.yuv\n" \


/**
 * GstSinkType:
 * @GST_WAYLANDSINK: Waylandsink Type.
 * @GST_VIDEO_ENCODE : Video Encode Type.
 * @GST_YUV_DUMP: YUV Filesink Type.
 * @GST_RTSP_STREAMING : RTSP streaming Type.
 * Type of App Sink.
 */
enum GstSinkType {
  GST_WAYLANDSINK,
  GST_YUV_DUMP,
  GST_VIDEO_ENCODE,
  GST_RTSP_STREAMING,
};
  
struct GstCameraAppCtx {
  GstElement *pipeline;
  GMainLoop *mloop;
  const gchar *output_file;
  gchar dev_video[16];
  enum GstSinkType sinktype;
  gint width;
  gint height;
  gint framerate;
  gchar *format;
};
typedef struct GstCameraAppCtx GstCameraAppContext;

/**
 * Handles interrupt by CTRL+C.
 *
 * @param userdata pointer to AppContext.
 * @return FALSE if the source should be removed, else TRUE.
 */
gboolean
handle_interrupt_signal (gpointer userdata)
{
  GstCameraAppContext *appctx = (GstCameraAppContext *) userdata;
  GstState state, pending;

  g_print ("\n\nReceived an interrupt signal, send EOS ...\n");

  if (!gst_element_get_state (
      appctx->pipeline, &state, &pending, GST_CLOCK_TIME_NONE)) {
    gst_printerr ("ERROR: get current state!\n");
    gst_element_send_event (appctx->pipeline, gst_event_new_eos ());
    return TRUE;
  }

  if (state == GST_STATE_PLAYING) {
    gst_element_send_event (appctx->pipeline, gst_event_new_eos ());
  } else {
    g_main_loop_quit (appctx->mloop);
  }
  return TRUE;
}

/**
 * Handles error events.
 *
 * @param bus Gstreamer bus for Mesaage passing in Pipeline.
 * @param message Gstreamer Error Event Message.
 * @param userdata Pointer to Main Loop for Handling Error.
 */
void
error_cb (GstBus * bus, GstMessage * message, gpointer userdata)
{
  GMainLoop *mloop = (GMainLoop*) userdata;
  GError *error = NULL;
  gchar *debug = NULL;

  gst_message_parse_error (message, &error, &debug);
  gst_object_default_error (GST_MESSAGE_SRC (message), error, debug);

  g_free (debug);
  g_error_free (error);

  g_main_loop_quit (mloop);
}

/**
 * Handles warning events.
 *
 * @param bus Gstreamer bus for Mesaage passing in Pipeline.
 * @param message Gstreamer Error Event Message.
 * @param userdata Pointer to Main Loop for Handling Error.
 */
void
warning_cb (GstBus * bus, GstMessage * message, gpointer userdata)
{
  GError *error = NULL;
  gchar *debug = NULL;

  gst_message_parse_warning (message, &error, &debug);
  gst_object_default_error (GST_MESSAGE_SRC (message), error, debug);

  g_free (debug);
  g_error_free (error);
}

/**
 * Handles End-Of-Stream(eos) Event.
 *
 * @param bus Gstreamer bus for Mesaage passing in Pipeline.
 * @param message Gstreamer eos Event Message.
 * @param userdata Pointer to Main Loop for Handling eos.
 */
void
eos_cb (GstBus * bus, GstMessage * message, gpointer userdata)
{
  GMainLoop *mloop = (GMainLoop*) userdata;

  g_print ("\nReceived End-of-Stream from '%s' ...\n",
      GST_MESSAGE_SRC_NAME (message));
  g_main_loop_quit (mloop);
}

/**
 * Handles state change events for the pipeline
 *
 * @param bus Gstreamer bus for Mesaage passing in Pipeline.
 * @param message Gstreamer eos Event Message.
 * @param userdata Pointer to Application Pipeline.
 */
void
state_changed_cb (GstBus * bus, GstMessage * message, gpointer userdata)
{
  GstElement *pipeline = GST_ELEMENT (userdata);
  GstState old, new_st, pending;

  // Handle state changes only for the pipeline.
  if (GST_MESSAGE_SRC (message) != GST_OBJECT_CAST (pipeline))
    return;

  gst_message_parse_state_changed (message, &old, &new_st, &pending);

  g_print ("\nPipeline state changed from %s to %s:\n",
      gst_element_state_get_name (old),
      gst_element_state_get_name (new_st));

  if ((new_st == GST_STATE_PAUSED) && (old == GST_STATE_READY) &&
      (pending == GST_STATE_VOID_PENDING)) {

    if (gst_element_set_state (pipeline,
        GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      gst_printerr (
          "\nPipeline doesn't want to transition to PLAYING state!\n");
      return;
    }
  }
}

/**
 * Create and initialize application context:
 *
 * @param NULL
 */
static GstCameraAppContext *
gst_app_context_new ()
{
  // Allocate memory for the new context
  GstCameraAppContext *ctx = (GstCameraAppContext *) g_new0 (GstCameraAppContext,
      1);

  // If memory allocation failed, print an error message and return NULL
  if (NULL == ctx) {
    g_printerr ("\n Unable to create App Context");
    return NULL;
  }

  // Initialize the context fields
  ctx->pipeline = NULL;
  ctx->mloop = NULL;
  ctx->output_file = NULL;
  ctx->width = DEFAULT_WIDTH;
  ctx->height = DEFAULT_HEIGHT;
  ctx->framerate = DEFAULT_FRAMERATE;
  ctx->format = g_strdup(DEFAULT_FORMAT);
  ctx->sinktype = GST_WAYLANDSINK;
  return ctx;
}

/**
 * Free Application context:
 *
 * @param appctx Application Context object
 */
static void
gst_app_context_free (GstCameraAppContext * appctx)
{
  // If specific pointer is not NULL, unref it

  if (appctx->mloop != NULL) {
    g_main_loop_unref (appctx->mloop);
    appctx->mloop = NULL;
  }

  if (appctx->pipeline != NULL) {
    gst_object_unref (appctx->pipeline);
    appctx->pipeline = NULL;
  }

  if (appctx != NULL)
    g_free (appctx);
}

/**
 * Find USB camera node:
 *
 * @param appctx Application Context object
 */
static gboolean
find_usb_camera_node (GstCameraAppContext * appctx)
{
  struct v4l2_capability v2cap;
  gint idx = 2, ret = 0, mFd = -1;

  while (idx < MAX_VID_DEV_CNT) {
    memset (appctx->dev_video, 0, sizeof (appctx->dev_video));

    // Skip for known nodes 32 and 33 are video driver nodes
    if (idx == 32 || idx == 33) {
      idx++;
      continue;
    }

    // start idx with 2 as 0 and 1 are already allocated nodes for camx
    ret = snprintf (appctx->dev_video, sizeof (appctx->dev_video), "/dev/video%d",
        idx);
    if (ret <= 0) {
      return FALSE;
    }

    g_print ("open USB camera device: %s\n", appctx->dev_video);
    mFd = open (appctx->dev_video, O_RDWR);
    if (mFd < 0) {
      mFd = -1;
      g_printerr ("Failed to open USB camera device: %s (%s)\n", appctx->dev_video,
          strerror (errno));
      idx++;
      continue;
    }

    if (ioctl (mFd, VIDIOC_QUERYCAP, &v2cap) == 0) {
      int capabilities;
      g_print ("ID_V4L_CAPABILITIES=:");

      if (v2cap.capabilities & V4L2_CAP_DEVICE_CAPS)
        capabilities = v2cap.device_caps;
      else
        capabilities = v2cap.capabilities;

      if ((capabilities & V4L2_CAP_VIDEO_CAPTURE) > 0 ||
          (capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) > 0) {
        g_print ("capture:");
      } else {
        g_printerr ("\n Failed to find USB camera try next Node\n");
        continue;
      }
    }
    break;
  }

  if (idx >= MAX_VID_DEV_CNT || mFd < 0 || ret < 0) {
    g_printerr ("Failed to open video device");
    close (mFd);
    return FALSE;
  }

  close (mFd);
  g_print ("open %s successful \n", appctx->dev_video);
  return TRUE;
}

/**
 * Create GST pipeline involves 3 main steps
 * 1. Create all elements/GST Plugins
 * 2. Set Paramters for each plugin
 * 3. Link plugins to create GST pipeline
 *
 * @param appctx Application Context Object.
 */
static gboolean
create_pipe (GstCameraAppContext * appctx)
{
  // Declare the elements of the pipeline
  GstElement *camerasrc, *capsfilter, *waylandsink, *convert, *filesink;
  GstCaps *filtercaps;
  gboolean ret = FALSE;

  // Create comman element
  camerasrc = gst_element_factory_make ("v4l2src", "camerasrc");
  capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  convert = gst_element_factory_make ("videoconvert", "convert");

  if (!camerasrc || !capsfilter || !convert) {
    g_printerr ("\n Not all elements could be created. Exiting.\n");
    return FALSE;
  }

  // Set properties for element
  g_object_set (G_OBJECT (camerasrc), "device", appctx->dev_video, NULL);

  // Set the source elements capability and in case YUV dump disable UBWC
  filtercaps = gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, appctx->format,
      "width", G_TYPE_INT, appctx->width,
      "height", G_TYPE_INT, appctx->height,
      "framerate", GST_TYPE_FRACTION, appctx->framerate, 1,
      NULL);

  g_object_set (G_OBJECT (capsfilter), "caps", filtercaps, NULL);
  gst_caps_unref (filtercaps);
  
  // check the sink type and create the sink elements
  if (appctx->sinktype == GST_WAYLANDSINK) {
    waylandsink = gst_element_factory_make ("waylandsink", "waylandsink");
    if (!waylandsink) {
      g_printerr ("\n waylandsink element not created. Exiting.\n");
      return FALSE;
    }
    g_object_set (G_OBJECT (waylandsink), "fullscreen", TRUE, NULL);

    gst_bin_add_many (GST_BIN (appctx->pipeline), camerasrc, capsfilter,
        convert, waylandsink, NULL);

    g_print ("\n Link pipeline for display elements ..\n");

    ret = gst_element_link_many (camerasrc, capsfilter, convert, waylandsink,
        NULL);
    if (!ret) {
      g_printerr ("\n Display Pipeline elements cannot be linked. Exiting.\n");
      gst_bin_remove_many (GST_BIN (appctx->pipeline), camerasrc, capsfilter,
          convert, waylandsink, NULL);
      return FALSE;
    }
  } else if (appctx->sinktype == GST_YUV_DUMP) {
    // set the output file location for filesink element
    appctx->output_file = DEFAULT_OP_YUV_FILENAME;
    filesink = gst_element_factory_make ("multifilesink", "filesink");
    if (!filesink) {
      g_printerr ("\n YUV dump elements could not be created. Exiting.\n");
      return FALSE;
    }
    g_object_set (G_OBJECT (filesink), "location", appctx->output_file, NULL);
    g_object_set (G_OBJECT (filesink), "enable-last-sample", FALSE, NULL);
    g_object_set (G_OBJECT (filesink), "max-files", 2, NULL);

    gst_bin_add_many (GST_BIN (appctx->pipeline), camerasrc, capsfilter,
        convert, filesink, NULL);

    g_print ("\n Link pipeline elements for yuv dump..\n");

    ret = gst_element_link_many (camerasrc, capsfilter, convert, filesink,
        NULL);
    if (!ret) {
      g_printerr ("\n Pipeline elements cannot be linked. Exiting.\n");
      gst_bin_remove_many (GST_BIN (appctx->pipeline), camerasrc, capsfilter,
          convert, filesink, NULL);
      return FALSE;
    }
  } 
    g_print ("\n All elements are linked successfully\n");
  return TRUE;
}

gint
main (gint argc, gchar *argv[])
{
  GOptionContext *ctx = NULL;
  GMainLoop *mloop = NULL;
  GstBus *bus = NULL;
  GstElement *pipeline = NULL;
  GstCameraAppContext *appctx = NULL;
  gboolean ret = FALSE;
  guint intrpt_watch_id = 0;

  // Setting Display environment variables
  setenv ("XDG_RUNTIME_DIR", "/dev/socket/weston", 0);
  setenv ("WAYLAND_DISPLAY", "wayland-1", 0);

  // create the application context
  appctx = gst_app_context_new ();
  if (appctx == NULL) {
    g_printerr ("\n Failed app context Initializing: Unknown error!\n");
    return -1;
  }

  // Configure input parameters
  GOptionEntry entries[] = {
    { "width", 'w', 0, G_OPTION_ARG_INT, &appctx->width,
      "width", "camera width" },
    { "height", 'h', 0, G_OPTION_ARG_INT, &appctx->height, "height",
      "camera height" },
    { "framerate", 'f', 0, G_OPTION_ARG_INT, &appctx->framerate, "framerate",
      "camera framerate" },
    { "format", 'F', 0, G_OPTION_ARG_STRING, &appctx->format, "Format", 
      "Video format (e.g., NV12, YUY2)" },
    { "output", 'o', 0, G_OPTION_ARG_INT, &appctx->sinktype,
      "Sinktype"
      "\n\t0-PREVIEW"
      "\n\t1-YUVDUMP" },
    { NULL }
    };

  // Parse command line entries.
  if ((ctx = g_option_context_new (GST_APP_SUMMARY)) != NULL) {
    gboolean success = FALSE;
    GError *error = NULL;

    g_option_context_add_main_entries (ctx, entries, NULL);
    g_option_context_add_group (ctx, gst_init_get_option_group ());

    success = g_option_context_parse (ctx, &argc, &argv, &error);
    g_option_context_free (ctx);

    if (!success && (error != NULL)) {
      g_printerr ("\n Failed to parse command line options: %s!\n",
          GST_STR_NULL (error->message));
      g_clear_error (&error);
      gst_app_context_free (appctx);
      return -1;
    } else if (!success && (NULL == error)) {
      g_printerr ("\n Initializing: Unknown error!\n");
      gst_app_context_free (appctx);
      return -1;
    }
  } else {
    g_printerr ("\n Failed to create options context!\n");
    gst_app_context_free (appctx);
    return -1;
  }

  // Initialize GST library.
  gst_init (&argc, &argv);

  // Create the pipeline
  pipeline = gst_pipeline_new ("pipeline");
  if (!pipeline) {
    g_printerr ("\n failed to create pipeline.\n");
    gst_app_context_free (appctx);
    return -1;
  }

  appctx->pipeline = pipeline;
  // Check for avaiable USB camera
  ret = find_usb_camera_node (appctx);
  if (!ret) {
    g_printerr ("\n Failed to find the USB camera.\n");
    gst_app_context_free (appctx);
    return -1;
  }

  // Build the pipeline
  ret = create_pipe (appctx);
  if (!ret) {
    g_printerr ("\n Failed to create GST pipe.\n");
    gst_app_context_free (appctx);
    return -1;
  }

  // Initialize main loop.
  if ((mloop = g_main_loop_new (NULL, FALSE)) == NULL) {
    g_printerr ("\n Failed to create Main loop!\n");
    gst_app_context_free (appctx);
    return -1;
  }
  appctx->mloop = mloop;

  // Retrieve reference to the pipeline's bus.
  if ((bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline))) == NULL) {
    g_printerr ("\n Failed to retrieve pipeline bus!\n");
    gst_app_context_free (appctx);
    return -1;
  }

  // Watch for messages on the pipeline's bus
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed_cb),
      pipeline);
  g_signal_connect (bus, "message::warning", G_CALLBACK (warning_cb), NULL);
  g_signal_connect (bus, "message::error", G_CALLBACK (error_cb), mloop);
  g_signal_connect (bus, "message::eos", G_CALLBACK (eos_cb), mloop);
  gst_object_unref (bus);

  // Register function for handling interrupt signals with the main loop
  intrpt_watch_id = g_unix_signal_add (SIGINT, handle_interrupt_signal, appctx);

  // Set the pipeline to the PAUSED state, On successful transition
  // move application state to PLAYING state in state_changed_cb function
  g_print ("\n Setting pipeline to PAUSED state ...\n");
  switch (gst_element_set_state (pipeline, GST_STATE_PAUSED)) {
    case GST_STATE_CHANGE_FAILURE:
      g_printerr ("\n Failed to transition to PAUSED state!\n");
      if (intrpt_watch_id)
        g_source_remove (intrpt_watch_id);
      gst_app_context_free (appctx);
      return -1;
    case GST_STATE_CHANGE_NO_PREROLL:
      g_print ("\n Pipeline is live and does not need PREROLL.\n");
      break;
    case GST_STATE_CHANGE_ASYNC:
      g_print ("\n Pipeline is PREROLLING ...\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      g_print ("\n Pipeline state change was successful\n");
      break;
  }

  // Start the main loop
  g_print ("\n Application is running... \n");

  g_main_loop_run (mloop);

  // Remove the interrupt signal handler
  if (intrpt_watch_id)
    g_source_remove (intrpt_watch_id);

  // Set the pipeline to the NULL state
  g_print ("\n Setting pipeline to NULL state ...\n");
  gst_element_set_state (appctx->pipeline, GST_STATE_NULL);
  if (appctx->output_file)
    g_print ("\n Video file will be stored at %s\n", appctx->output_file);

  // Free the application context
  g_print ("\n Free the Application context\n");
  gst_app_context_free (appctx);

  // Deinitialize the GST library
  g_print ("\n gst_deinit\n");
  gst_deinit ();

  return 0;
}
