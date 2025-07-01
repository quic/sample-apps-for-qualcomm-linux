/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <glib-unix.h>
#include <gst/gst.h>
#include <glib.h>

#define DEFAULT_FILESOURCE "/opt/video.mp4"

#define GST_APP_SUMMARY "This app enables the users to do color convert" \
    " and scaling using openGL. \n" \
    "\nCommand:\n" \
    "  gst-test-app --infile=/opt/video.mp4 \n" \
    "  or \n" \
    "  gst-test-app \n" \

/**
  * GstAppContext:
  * @pipeline: Pipeline connecting all the elements for Use Case.
  * @plugins : List of all the plugins used in Pipeline.
  * @mloop   : Main Loop for the Application.
  *
  * Application context to pass information between the functions.
  */
typedef struct
{
  // Pointer to the pipeline
  GstElement *pipeline;
  // Pointer to the mainloop
  GMainLoop  *loop;
  //Input sources
  gchar *input_file;
} GstAppContext;

/**
  * Create and initialize application context:
  *
  * @param NULL
  */
static GstAppContext *
gst_app_context_new ()
{
    // Allocate memory for the new context
    GstAppContext *ctx = (GstAppContext *) g_new0 (GstAppContext, 1);

    if (NULL == ctx) {
      g_printerr ("\n Unable to create App Context\n");
      return NULL;
    }

    // Initialize the context fields
    ctx->pipeline = NULL;
    ctx->loop = NULL;
    ctx->input_file = g_strdup (DEFAULT_FILESOURCE);

    return ctx;
}

 /**
  * Free Application context:
  *
  * @param appctx application context object
  */
static void
gst_app_context_free (GstAppContext * appctx)
{
    // If specific pointer is not NULL, unref it
    if (appctx->loop != NULL) {
      g_main_loop_unref (appctx->loop);
      appctx->loop = NULL;
    }

    if (appctx->pipeline != NULL) {
      gst_object_unref (appctx->pipeline);
      appctx->pipeline = NULL;
    }

    if (appctx->input_file != NULL)
      g_free (appctx->input_file);

    if (appctx != NULL)
      g_free ((gpointer)appctx);
}

 /**
  * Handles interrupt by CTRL+C.
  *
  * @param userdata pointer to AppContext.
  * @return FALSE if the source should be removed, else TRUE.
  */
gboolean
handle_interrupt_signal (gpointer userdata)
{
    GstAppContext *appctx = (GstAppContext *) userdata;
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
      g_main_loop_quit (appctx->loop);
    }
    return TRUE;
}

/**
  * Handles End-Of-Stream(eos) Event.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer eos Event Message.
  * @param userdata Pointer to Main Loop for Handling eos.
  */
static
gboolean eos_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    g_print ("End of stream reached.\n");
    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}

/**
  * Handles error events.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer Error Event Message.
  * @param userdata Pointer to Main Loop for Handling Error.
  */
static
gboolean error_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;

    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

    g_clear_error (&err);
    g_free (debug_info);

    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}

/**
  * Handles warning events.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer Error Event Message.
  * @param userdata Pointer to Main Loop for Handling Error.
  */
static
gboolean warning_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;

    gst_message_parse_warning (msg, &err, &debug_info);
    g_printerr ("Warning from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

    g_clear_error (&err);
    g_free (debug_info);
    return TRUE;
}

static
void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *queue = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_static_pad(queue, "sink");

    if (!gst_pad_is_linked(sinkpad)) {
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr("Failed to link dynamic pad.\n");
        } else {
            g_print("Dynamic pad linked successfully.\n");
        }
    }

    gst_object_unref(sinkpad);
}

gint
main (gint argc, gchar *argv[]) {
    GstAppContext *appctx = NULL;
    GstElement *source, *demuxer, *vparse, *decoder, *upload, *scale;
    GstElement *videoconvert, *convert, *download, *filter, *sink, *pqueue;
    GstCaps *caps;
    GstBus *bus;
    guint intrpt_watch_id = 0;

    // Setting Display environment variables
    setenv ("XDG_RUNTIME_DIR", "/dev/socket/weston", 0);
    setenv ("WAYLAND_DISPLAY", "wayland-1", 0);

    // create the app context
    appctx = gst_app_context_new ();
    if (NULL == appctx) {
      g_printerr ("\n Failed app context Initializing: Unknown error!\n");
      return -1;
    }

    GOptionEntry entries[] = {
        { "infile", 0, 0, G_OPTION_ARG_FILENAME, (gpointer) &appctx->input_file,
          "First video file", "FILE" },
        { NULL }
    };

    GOptionContext *context = g_option_context_new (GST_APP_SUMMARY);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group());

    if (!g_option_context_parse (context, &argc, &argv, NULL)) {
        g_printerr ("Failed to parse options.\n");
        return -1;
    }
    g_option_context_free (context);

    gst_init (&argc, &argv);

    // Create the elements
    source = gst_element_factory_make ("filesrc", "source");
    demuxer = gst_element_factory_make ("qtdemux", "demuxer");
    vparse = gst_element_factory_make ("h264parse", "vparse");
    decoder = gst_element_factory_make ("v4l2h264dec", "decoder");
    videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
    upload = gst_element_factory_make ("glupload", "upload");
    scale = gst_element_factory_make ("glcolorscale", "scale");
    convert = gst_element_factory_make ("glcolorconvert", "convert");
    download = gst_element_factory_make ("gldownload", "download");
    filter = gst_element_factory_make ("capsfilter", "filter");
    sink = gst_element_factory_make ("waylandsink", "sink");
    pqueue = gst_element_factory_make ("queue", "pqueue");

    // Create the empty pipeline
    appctx->pipeline = gst_pipeline_new("transform-pipeline");

    if (!appctx->pipeline || !source || !demuxer || !decoder ||
        !videoconvert || !upload || !scale || !convert || !download ||
        !filter || !sink) {
        g_printerr("Not all elements could be created testting.\n");
        return -1;
    }

    caps = gst_caps_new_simple ("video/x-raw",
                                 "width", G_TYPE_INT, 320,
                                 "height", G_TYPE_INT, 240,
                                 NULL);
    g_object_set (filter, "caps", caps, NULL);
    gst_caps_unref (caps);


    // Set the properties of the filesrc
    g_object_set (source, "location", appctx->input_file, NULL);

    // Set display properties
    g_object_set (G_OBJECT (sink), "fullscreen", true, NULL);

    // Build the pipeline
    gst_bin_add_many (GST_BIN (appctx->pipeline), source, demuxer, vparse,
        decoder, videoconvert, upload, scale, convert, download, filter,
        sink, pqueue, NULL);

    if (gst_element_link (source, demuxer) != TRUE ||
        gst_element_link_many (pqueue, vparse, decoder, videoconvert,
            upload, scale, convert, download, filter, sink, NULL) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_app_context_free (appctx);
        return -1;
    }

    // Connect demuxer pad-added signal
    g_signal_connect (demuxer, "pad-added", G_CALLBACK(on_pad_added), pqueue);

    appctx->loop = g_main_loop_new (NULL, FALSE);

    // Watch for messages on the pipeline's bus.
    bus = gst_element_get_bus (appctx->pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message::error", G_CALLBACK(error_cb), appctx->loop);
    g_signal_connect (bus, "message::warning", G_CALLBACK(warning_cb), NULL);
    g_signal_connect (bus, "message::eos", G_CALLBACK(eos_cb), appctx->loop);
    gst_object_unref (bus);

    // Register function for handling interrupt signals with the main loop.
    intrpt_watch_id = g_unix_signal_add (SIGINT, handle_interrupt_signal, appctx);

    // Set the pipeline to the PAUSED state, On successful transition
    // move application state to PLAYING state in state_changed_cb function
    g_print ("Setting pipeline to PLAYING state ...\n");
    switch (gst_element_set_state (appctx->pipeline, GST_STATE_PLAYING)) {
      case GST_STATE_CHANGE_FAILURE:
        g_printerr ("\n Failed to transition to PLAYING state!\n");
        if(intrpt_watch_id)
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

    g_print ("\n Application is running... \n");
    g_main_loop_run (appctx->loop);

    if(intrpt_watch_id)
      g_source_remove (intrpt_watch_id);

    g_print ("\n Setting pipeline to NULL state ...\n");
    gst_element_set_state(appctx->pipeline, GST_STATE_NULL);

    gst_app_context_free (appctx);

    gst_deinit ();

    g_print ("Application End Successfully\n");

    return 0;
}

