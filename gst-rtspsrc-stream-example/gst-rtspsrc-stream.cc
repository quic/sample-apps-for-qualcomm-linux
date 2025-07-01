/*
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
 
/*
* Gstreamer Application:
* Gstreamer Application for IP Camera usecases rtspsrc as input.
*
* Description:
* This application Demonstrates IP camera usecases with below possible outputs:
*     --Live IP Camera Preview on Display
*
* Usage:
* For RTSPSRC PREVIEW on device:
* gst-rtspsrc-stream-example "<rtspsrc URL>"
*
* eg: gst-rtspsrc-stream-example "rtsp://admin:qualcomm1@192.168.1.250:554/cam/realmonitor?channel=1&subtype=0"
* 
* Help:
* gst-rtspsrc-stream-example --help
*
* Tips:
* Always enclose the RTSP URI in double quotes if it contains special characters like '&'.
* Press Ctrl+C to stop the stream gracefully.
* Requires Wayland-compatible video sink.
 
* *******************************************************************************
* IP Camera Preview on Display:
*     qtiqmmfsrc->capsfilter->waylandsink
* *******************************************************************************
*/
 
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>

#define GST_APP_SUMMARY "This application Demonstrates IP camera usecases \n" \
                        "  as rtspsource preview stream as outputs \n" \
                "For streaming: gst-rtspsrc-stream-example --src-uri=\"<URL>\" \n"
 
/**
  * GstAppContext:
  * @pipeline: Pipeline connecting all the elements for Use Case.
  * @mloop   : Main Loop for the Application.
  * @uri     : rtsp source uri
  *
  * Application context to pass information between the functions.
  */
typedef struct
{
  // Pointer to the pipeline
  GstElement *pipeline;
  // Pointer to the mainloop
  GMainLoop  *loop;
  gchar *url;
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
    ctx->url = NULL;

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
 
    if (appctx != NULL)
      g_free ((gpointer)appctx);

    if (appctx->url != NULL)
      g_free (appctx->url);

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

  g_print ("\n Pipeline state changed from %s to %s:\n",
      gst_element_state_get_name (old),
      gst_element_state_get_name (new_st));

  if ((new_st == GST_STATE_PAUSED) && (old == GST_STATE_READY) &&
      (pending == GST_STATE_VOID_PENDING)) {

    if (gst_element_set_state (pipeline,
        GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      gst_printerr (
          "\n Pipeline doesn't want to transition to PLAYING state!\n");
      return;
    }
  }
}
 
 
static gboolean eos_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    g_print ("\n End of stream reached.\n");
    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}
 
static gboolean error_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;
 
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("\n Error from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("\n Debug info: %s\n", debug_info ? debug_info : "none");
 
    g_clear_error (&err);
    g_free (debug_info);
 
    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}
 
static gboolean warning_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;
 
    gst_message_parse_warning (msg, &err, &debug_info);
    g_printerr ("\n Warning from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("\n Debug info: %s\n", debug_info ? debug_info : "none");
 
    g_clear_error (&err);
    g_free (debug_info);
    return TRUE;
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
 
    g_print ("\n\n Received an interrupt signal, send EOS ...\n");
 
    if (!gst_element_get_state (
        appctx->pipeline, &state, &pending, GST_CLOCK_TIME_NONE)) {
      gst_printerr ("\n ERROR: get current state!\n");
    }
    
    gst_element_send_event (appctx->pipeline, gst_event_new_eos ());
 
    if (appctx->loop) {
      g_main_loop_quit (appctx->loop);
    }

    return TRUE;
}


// Callback to link dynamic pad from rtspsrc
static void
on_pad_added(GstElement *src, GstPad *new_pad, gpointer user_data) {
   GstElement *queue = (GstElement *)user_data;
   GstPad *sink_pad = gst_element_get_static_pad(queue, "sink");

   if (gst_pad_is_linked(sink_pad)) {
       gst_object_unref(sink_pad);
       return;
   }
   if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
       g_printerr("\n Failed to link dynamic pad.\n");
   } else {
       g_print("\n Linked dynamic pad to queue.\n");
   }
   gst_object_unref(sink_pad);
}

int main(int argc, char *argv[]) {
   GstAppContext *appctx = {0};
   GstElement *source, *queue1, *depay, *parse, *decoder;
   GstElement *queue2, *capsfilter, *fpsdisplaysink, *waylandsink;
   GstCaps *caps;
   GstBus *bus;
   guint intrpt_watch_id = 0;
   GError *error = NULL;

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
     { "src-uri", 0, 0, G_OPTION_ARG_STRING, &appctx->url, "RTSP source URI", "URL" },
     { NULL }
   };

   GOptionContext *context = g_option_context_new (GST_APP_SUMMARY);
   g_option_context_add_main_entries (context, entries, NULL);
   g_option_context_add_group (context, gst_init_get_option_group());

   if (!g_option_context_parse (context, &argc, &argv, &error)) {
       g_printerr ("\n Option parsing failed: %s\n", error->message);
       g_clear_error (&error);
       g_option_context_free (context);
       gst_app_context_free (appctx);
       return -1;
   }
   
   if (!appctx->url || g_strcmp0(appctx->url, "") == 0) {
      g_print("\n Error: Please provide the RTSP URI using --src-uri=\"rtsp://...\" \n");
      gst_app_context_free (appctx);
      return -1;
   }

   g_print(" Using URI: %s\n", appctx->url);
   g_option_context_free (context);
 
   // Initialize GST library.
   gst_init (&argc, &argv);

   // Create elements
   appctx->pipeline = gst_pipeline_new("rtsp-pipeline");
   source           = gst_element_factory_make("rtspsrc", "source");
   queue1           = gst_element_factory_make("queue", "queue1");
   depay            = gst_element_factory_make("rtph264depay", "depay");
   parse            = gst_element_factory_make("h264parse", "parse");
   decoder          = gst_element_factory_make("v4l2h264dec", "decoder");
   queue2           = gst_element_factory_make("queue", "queue2");
   capsfilter       = gst_element_factory_make("capsfilter", "capsfilter");
   waylandsink      = gst_element_factory_make("waylandsink", "waylandsink");
   fpsdisplaysink   = gst_element_factory_make("fpsdisplaysink", "fpsdisplaysink");
   
   if (!appctx->pipeline || !source || !queue1 || !depay || !parse ||
       !decoder || !queue2 || !capsfilter || !fpsdisplaysink ) {
       g_printerr("Failed to create GStreamer elements.\n");
       gst_app_context_free (appctx);
       return -1;
   }
 
   // Configure elements
   g_object_set(source, "location", appctx->url, NULL);
   g_object_set(decoder, "capture-io-mode", 4, "output-io-mode", 4, NULL);
   g_object_set(fpsdisplaysink, "sync", FALSE,
                "signal-fps-measurements", TRUE,
                "text-overlay", TRUE,
                "video-sink", waylandsink,
                NULL);
 
   // Set caps
   caps = gst_caps_from_string("video/x-raw,format=NV12");
   g_object_set(capsfilter, "caps", caps, NULL);
   gst_caps_unref(caps);
 
   // Add elements to pipeline
   gst_bin_add_many(GST_BIN(appctx->pipeline), source, queue1, depay, parse,
                    decoder, queue2, capsfilter, fpsdisplaysink, NULL);
 
   // Link static elements
   if (!gst_element_link_many(queue1, depay, parse, decoder, queue2,
       capsfilter, fpsdisplaysink, NULL)) {
       g_printerr("\n Failed to link static elements.\n");
       gst_app_context_free (appctx);
       return -1;
   }
 
   // Connect pad-added signal
   g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), queue1);

   // Register function for handling interrupt signals with the main loop.
   intrpt_watch_id = g_unix_signal_add (SIGINT, handle_interrupt_signal, appctx);

   // Set the pipeline to the PAUSED state, On successful transition
   // move application state to PLAYING state in state_changed_cb function
   g_print ("\n Setting pipeline to PAUSED state ...\n");
   switch (gst_element_set_state (appctx->pipeline, GST_STATE_PAUSED)) {
    case GST_STATE_CHANGE_FAILURE:
      g_printerr ("\n Failed to transition to PAUSED state!\n");
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

   // Start pipeline
   gst_element_set_state(appctx->pipeline, GST_STATE_PLAYING);
   appctx->loop = g_main_loop_new(NULL, FALSE);
   
   g_print ("\n Application is running...\n");
   g_main_loop_run(appctx->loop);
   
   bus = gst_element_get_bus (appctx->pipeline);
   gst_bus_add_signal_watch (bus);
   
   g_signal_connect (bus, "message::error", G_CALLBACK(error_cb), appctx->loop);
   g_signal_connect (bus, "message::warning", G_CALLBACK(warning_cb), NULL);
   g_signal_connect (bus, "message::eos", G_CALLBACK(eos_cb), appctx->loop);
 
   // Cleanup
   g_source_remove (intrpt_watch_id);
   
   g_print ("\n Setting pipeline to NULL state ...\n");
   gst_element_set_state(appctx->pipeline, GST_STATE_NULL);
   gst_object_unref (bus);

   g_print ("\n Free the Application context\n");
   gst_app_context_free (appctx);

   g_print ("\n gst_deinit\n");
   gst_deinit ();

 
   return 0;
}
