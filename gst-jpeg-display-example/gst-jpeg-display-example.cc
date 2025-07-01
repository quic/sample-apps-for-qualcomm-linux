/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <glib-unix.h>
#include <gst/gst.h>
#include <glib.h>

#define DEFAULT_INPUT_IMAGESOURCE "/opt/frame%d.jpg"

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
  gchar *location;
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
    ctx->location = g_strdup (DEFAULT_INPUT_IMAGESOURCE);

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

    if (appctx->location != NULL)
      g_free (appctx->location);

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

gint
main (int argc, char *argv[]) {
    GstAppContext *appctx = NULL;
    GstElement *src, *jpegdec, *videoconvert, *sink;
    GstBus *bus;
    GError *error = NULL;
    guint intrpt_watch_id = 0;

    // create the app context
    appctx = gst_app_context_new ();
    if (NULL == appctx) {
      g_printerr ("\n Failed app context Initializing: Unknown error!\n");
      return -1;
    }

    // Setting Display environment variables
    setenv ("XDG_RUNTIME_DIR", "/dev/socket/weston", 0);
    setenv ("WAYLAND_DISPLAY", "wayland-1", 0);

    GOptionEntry entries[] = {
        {"location", 'l', 0, G_OPTION_ARG_STRING, &appctx->location,
         "Image sequence location (e.g., /opt/frame%d.jpg)",
	 "PATH" },
        { NULL }
    };

    GOptionContext *context = g_option_context_new ("- GStreamer JPEG viewer");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group());

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        g_clear_error (&error);
        g_option_context_free (context);
	gst_app_context_free (appctx);
        return -1;
    }

    g_option_context_free (context);

    //Initialize GST library.
    gst_init (&argc, &argv);

    if (!appctx->location) {
        appctx->location = g_strdup (DEFAULT_INPUT_IMAGESOURCE);
    }

    // Create elements
    src = gst_element_factory_make ("multifilesrc", "src");
    jpegdec = gst_element_factory_make ("jpegdec", "jpegdec");
    videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
    sink = gst_element_factory_make ("waylandsink", "sink");

    appctx->pipeline = gst_pipeline_new ("jpeg-display-pipeline");

    if (!appctx->pipeline || !src || !jpegdec || !videoconvert || !sink) {
        g_printerr ("Failed to create elements.\n");
        g_free (appctx->location);
	gst_app_context_free (appctx);
        return -1;
    }

    g_object_set (src, "location", appctx->location, NULL);
    g_free (appctx->location);

    gst_bin_add_many (GST_BIN (appctx->pipeline), src, jpegdec, videoconvert, sink, NULL);
    if (!gst_element_link_many (src, jpegdec, videoconvert, sink, NULL)) {
        g_printerr ("Failed to link elements.\n");
        gst_object_unref (appctx->pipeline);
	gst_app_context_free (appctx);
        return -1;
    }

    appctx->loop = g_main_loop_new (NULL, FALSE);

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

    g_print ("\n Setting pipeline to NULL state ...\n");
    gst_element_set_state (appctx->pipeline, GST_STATE_NULL);

    gst_app_context_free (appctx);
    gst_deinit ();
    g_print ("Application End Successfully\n");

    return 0;
}

