/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <glib-unix.h>
#include <gst/gst.h>
#include <glib.h>

#define DEFAULT_FILE1 "/opt/video.mp4"
#define DEFAULT_FILE2 "/opt/video.mp4"

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
  gchar *file1;
  gchar *file2;
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
    ctx->file1 = g_strdup (DEFAULT_FILE1);
    ctx->file2 = g_strdup (DEFAULT_FILE2);

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

    if (appctx->file1 != NULL)
      g_free (appctx->file1);

    if (appctx->file2 != NULL)
      g_free (appctx->file2);

    if (appctx != NULL)
      g_free ((gpointer)appctx);
}

 /**
  * Function to link the dynamic video pad of demux to queue:
  *
  * @param element GStreamer source element
  * @param pad GStreamer source element pad
  * @param data sink element object
  */
static void
on_pad_added (GstElement *src, GstPad *new_pad, gpointer data) {
    GstElement *parser = (GstElement *) data;
    GstPad *sink_pad = gst_element_get_static_pad (parser, "sink");

    if (gst_pad_is_linked (sink_pad)) {
        g_object_unref (sink_pad);
        return;
    }

    if (gst_pad_link (new_pad, sink_pad) != GST_PAD_LINK_OK) {
        g_printerr ("Failed to link dynamic pad.\n");
    } else {
        g_print ("Dynamic pad linked.\n");
    }

    g_object_unref (sink_pad);
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

int
main (int argc, char *argv[]) {
    GstAppContext *appctx = NULL;
    GstElement *src1, *demux1, *parse1, *dec1, *conv1;
    GstElement *src2, *demux2, *parse2, *dec2, *conv2, *compositor, *sink;
    GstBus *bus;
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
        { "file1", 0, 0, G_OPTION_ARG_FILENAME, (gpointer) &appctx->file1,
          "First video file", "FILE" },
        { "file2", 0, 0, G_OPTION_ARG_FILENAME, (gpointer) &appctx->file2, 
          "Second video file", "FILE" },
        { NULL }
    };

    GOptionContext *context = g_option_context_new ("- video compositor");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group());

    if (!g_option_context_parse (context, &argc, &argv, NULL)) {
        g_printerr ("Failed to parse options.\n");
        return -1;
    }
    g_option_context_free (context);

    //Initialize GST library.
    gst_init (&argc, &argv);

    appctx->pipeline = gst_pipeline_new ("composite-pipeline");

    src1 = gst_element_factory_make ("filesrc", "src1");
    demux1 = gst_element_factory_make ("qtdemux", "demux1");
    parse1 = gst_element_factory_make ("h264parse", "parse1");
    dec1 = gst_element_factory_make ("v4l2h264dec", "dec1");
    conv1 = gst_element_factory_make("videoconvert", "conv1");

    src2 = gst_element_factory_make ("filesrc", "src2");
    demux2 = gst_element_factory_make ("qtdemux", "demux2");
    parse2 = gst_element_factory_make ("h264parse", "parse2");
    dec2 = gst_element_factory_make ("v4l2h264dec", "dec2");
    conv2 = gst_element_factory_make ("videoconvert", "conv2");

    compositor = gst_element_factory_make ("compositor", "compositor");
    sink = gst_element_factory_make ("waylandsink", "sink");

    if (!appctx->pipeline || !src1 || !demux1 || !parse1 || !dec1 || !conv1 ||
        !src2 || !demux2 || !parse2 || !dec2 || !conv2 ||
        !compositor || !sink) {
        g_printerr ("Failed to create elements.\n");
        return -1;
    }

    // Set display properties
    g_object_set (G_OBJECT (sink), "fullscreen", true, NULL);

    // Set filesrc properties
    g_object_set (src1, "location", appctx->file1, NULL);
    g_object_set (src2, "location", appctx->file2, NULL);

    gst_bin_add_many (GST_BIN(appctx->pipeline),
        src1, demux1, parse1, dec1, conv1,
        src2, demux2, parse2, dec2, conv2,
        compositor, sink, NULL);

    gst_element_link (src1, demux1);
    gst_element_link (src2, demux2);

    gst_element_link_many (parse1, dec1, conv1, NULL);
    gst_element_link_many (parse2, dec2, conv2, NULL);
    gst_element_link (compositor, sink);

    g_signal_connect (demux1, "pad-added", G_CALLBACK (on_pad_added), parse1);
    g_signal_connect (demux2, "pad-added", G_CALLBACK (on_pad_added), parse2);

    GstPad *sinkpad1 = gst_element_request_pad_simple (compositor, "sink_%u");
    GstPad *srcpad1 = gst_element_get_static_pad (conv1, "src");
    gst_pad_link (srcpad1, sinkpad1);
    g_object_set (sinkpad1, "xpos", 0, "ypos", 0, NULL);

    GstPad *sinkpad2 = gst_element_request_pad_simple (compositor, "sink_%u");
    GstPad *srcpad2 = gst_element_get_static_pad (conv2, "src");
    gst_pad_link (srcpad2, sinkpad2);
    g_object_set (sinkpad2, "xpos", 640, "ypos", 0, NULL);

    gst_object_unref (sinkpad1);
    gst_object_unref (sinkpad2);
    gst_object_unref (srcpad1);
    gst_object_unref (srcpad2);

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
    gst_element_set_state (appctx->pipeline, GST_STATE_NULL);

    gst_app_context_free (appctx);

    gst_deinit ();

    g_print ("Application End Successfully\n");

    return 0;
}
