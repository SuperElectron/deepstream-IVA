#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <glib.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "Processing.h"
#include "gstnvdsmeta.h"
#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "pipelineUtils.hpp"


/**
 * @file callback.hpp
 * @description gstreamer callbacks and probes
 * @copydoc https://gstreamer.freedesktop.org/documentation/gstreamer/gstpad.html?gi-language=c#GstFlowReturn
 */

using njson = nlohmann::json;
using namespace processUtils;

namespace core {


inline cv::Mat getRGBFrame(GstMapInfo in_map_info, gint idx) {

  // To access the frame
  NvBufSurface *surface = NULL;
  NvBufSurface surface_idx;
  NvBufSurfTransformRect src_rect, dst_rect;

  surface = (NvBufSurface *) in_map_info.data;
  surface_idx = *surface;
  surface_idx.surfaceList = &(surface->surfaceList[idx]);
  surface_idx.numFilled = surface_idx.batchSize = 1;

  int batch_size = surface_idx.batchSize;
  src_rect.top   = 0;
  src_rect.left  = 0;
  src_rect.width = (guint) surface->surfaceList[idx].width;
  src_rect.height= (guint) surface->surfaceList[idx].height;

  dst_rect.top   = 0;
  dst_rect.left  = 0;
  dst_rect.width = (guint) surface->surfaceList[idx].width;
  dst_rect.height= (guint) surface->surfaceList[idx].height;

  NvBufSurfTransformParams nvbufsurface_params;
  nvbufsurface_params.src_rect = &src_rect;
  nvbufsurface_params.dst_rect = &dst_rect;
  nvbufsurface_params.transform_flag =  NVBUFSURF_TRANSFORM_CROP_SRC | NVBUFSURF_TRANSFORM_CROP_DST;
  nvbufsurface_params.transform_filter = NvBufSurfTransformInter_Default;

  NvBufSurface *dst_surface = NULL;
  NvBufSurfaceCreateParams nvbufsurface_create_params;

  nvbufsurface_create_params.gpuId  = surface->gpuId;
  nvbufsurface_create_params.width  = (guint) surface->surfaceList[idx].width;
  nvbufsurface_create_params.height = (guint) surface->surfaceList[idx].height;
  nvbufsurface_create_params.size = 0;
  // nvbufsurface_create_params.isContiguous = true;
  nvbufsurface_create_params.colorFormat = NVBUF_COLOR_FORMAT_RGBA;
  nvbufsurface_create_params.layout = NVBUF_LAYOUT_PITCH;

  // THE memType PARAM IS SET TO CUDA UNIFIED IN dGPU DEVICES COMMENT IT out
  // AND USE THE IMMEDIATE NEXT LINE TO SET THE memType PARAM FOR JETSON DEVICES
#ifdef PLATFORM_TEGRA
  LOG(WARNING) << "Setting nvbufsurface_create_params.memType = NVBUF_MEM_DEFAULT";
  nvbufsurface_create_params.memType = NVBUF_MEM_DEFAULT;
#else
  LOG(WARNING) << "Setting nvbufsurface_create_params.memType = NVBUF_MEM_CUDA_UNIFIED";
  nvbufsurface_create_params.memType = NVBUF_MEM_CUDA_UNIFIED;
#endif

  cudaError_t cuda_err = cudaSetDevice (surface->gpuId);
  cudaStream_t cuda_stream;
  cuda_err = cudaStreamCreate(&cuda_stream);
  int create_result = NvBufSurfaceCreate(&dst_surface, batch_size, &nvbufsurface_create_params);

  NvBufSurfTransformConfigParams transform_config_params;
  NvBufSurfTransform_Error err;
  transform_config_params.compute_mode = NvBufSurfTransformCompute_Default;
  transform_config_params.gpu_id = surface->gpuId;
  transform_config_params.cuda_stream = cuda_stream;
  err = NvBufSurfTransformSetSessionParams (&transform_config_params);

  NvBufSurfaceMemSet (dst_surface, 0, 0, 0);

  err = NvBufSurfTransform (&surface_idx, dst_surface, &nvbufsurface_params);

  if (err != NvBufSurfTransformError_Success) {
    g_print ("NvBufSurfTransform failed with error %d while converting buffer\n", err);
  }

  NvBufSurfaceMap (dst_surface, 0, 0, NVBUF_MAP_READ);
  NvBufSurfaceSyncForCpu (dst_surface, 0, 0);

  cv::Mat bgr_frame = cv::Mat (cv::Size(nvbufsurface_create_params.width, nvbufsurface_create_params.height), CV_8UC3);

  cv::Mat in_mat = cv::Mat (nvbufsurface_create_params.height, nvbufsurface_create_params.width, CV_8UC4, dst_surface->surfaceList[0].mappedAddr.addr[0], dst_surface->surfaceList[0].pitch);

  cv::cvtColor (in_mat, bgr_frame, cv::COLOR_RGBA2BGR);
  NvBufSurfaceUnMap(dst_surface, 0 , 0);
  NvBufSurfaceDestroy(dst_surface);
  cudaStreamDestroy (cuda_stream);

  return bgr_frame;
}

/**
 * @brief static callbacks to be used in Pipeline module
 * @copydoc Example of setting a callback (pad_probe) to process metadata
 *   gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
 * core::GstCallbacks::print_metadata, (gpointer) NULL, NULL);
 *   gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
 * core::GstCallbacks::print_metadata, (gpointer) Class, NULL);
 *   gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
 * core::GstCallbacks::print_metadata, (gpointer) variable, NULL);
 *
 */
namespace GstCallbacks {

/**
 * @brief callback for nvidia element pad (sink of nvtracker) that extracts metadata from pipeline and adds processes it
 * and creates a kafka payload with its items
 * @copydoc adds to meta_queue and display_queue if enabled in config.json
 *
 * @param pad               the pad to which the callback is attached
 * @param info              the data component of the gstreamer buffer
 * @param u_data            user data pointer passed into the callback
 * @return GstFlowReturn    return handle behaviour
 */
inline GstPadProbeReturn probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  // unpack pointer
  auto processor = (core::Processing *)u_data;
  // NvDS structures
  bool ret = processor->probe_callback(pad, info);
  if (!ret) {
    LOG(ERROR) << "Processing failed";
    GST_PAD_PROBE_PASS;
  }


  return GST_PAD_PROBE_OK;
}

/**
 * @brief callback to write on display for element pad that has caps="video/x-raw,format=YV12"
 * @copydoc reads from _display_queue and writes bbox onto image
 *
 * @param pad               the pad to which the callback is attached
 * @param info              the data component of the gstreamer buffer
 * @param u_data            user data pointer passed into the callback
 * @return GstFlowReturn    return handle behaviour
 */
inline GstPadProbeReturn osd_callback(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  // unpack pointer
  auto processor = (core::Processing *)u_data;
  // NvDS structures
  bool ret = processor->osd_callback(pad, info);
  if (!ret) {
    LOG(ERROR) << "Processing failed";
    GST_PAD_PROBE_PASS;
  }

  return GST_PAD_PROBE_OK;
}

//////////////// DEBUGGING CALLBACKS ////////////////

/**
 * @brief callback to convert any buffer to image with openCV
 *
 * @param pad               the pad to which the callback is attached
 * @param info              the data component of the gstreamer buffer
 * @param u_data            user data pointer passed into the callback
 * @return GstFlowReturn    return handle behaviour
 */
inline GstPadProbeReturn save_image_to_disk(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  static unsigned long int counter = 0;
  std::string img_name = "frame_";

  // unpack pointer
  auto processor = (core::Processing *)u_data;
  // get width, height and format (e.g. video/x-raw) from GstBuffer
  int width, height;
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps)
    caps = gst_pad_query_caps(pad, NULL);

  GstStructure *s = gst_caps_get_structure(caps, 0);
  bool res = gst_structure_get_int(s, "width", &width);
  res |= gst_structure_get_int(s, "height", &height);
  std::string video_format = (std::string)gst_structure_get_string(s, "format");

  // DISPLAY CAPS (for debugging)
  //	for (int i = 0; i < gst_caps_get_size (caps); i++) {
  //		GstStructure *structure = gst_caps_get_structure (caps, i);
  //
  //		g_print ("%s\n", gst_structure_get_name (structure));
  //		gst_structure_foreach (structure, processUtils::print_field,
  //(gpointer) NULL);
  //	}

  gst_caps_unref(caps);
  if (!res)
    LOG(FATAL) << "The pad doesn't have image dimensions!";
  VLOG(DEBUG) << "GST_EVENT_CAPS (video_format=" << video_format << ",width=" << width << ",height=" << height << ")";

  // gather buffer contents
  GstBuffer *buffer;
  GstMapInfo map;
  buffer = (GstBuffer *)info->data;
  if (!gst_buffer_map(buffer, &map, GST_MAP_READWRITE))  // GST_MAP_READWRITE
  {
    LOG(ERROR) << "buffer GstMapInfo is not readable";
    return GST_PAD_PROBE_PASS;
  }

  /**
   * https://stackoverflow.com/questions/65308163/converting-gstreamer-yuv-420-i420-raw-frame-to-opencv-cvmat
   * // YUV ==> cv::Mat frame(cv::Size(width, height), CV_8UC3, (char
   * *)map.data, (width * 3) + 2);
   */
  img_name += std::to_string(counter) + (std::string) ".png";

  if (video_format.compare("RGB") == 0) {
    VLOG(DEBUG) << "Detected RGB caps format=" << video_format;
    cv::Mat input_frame(cv::Size(width, height), CV_8UC3, (char *)map.data, cv::Mat::AUTO_STEP);
    cv::imwrite(img_name.c_str(), input_frame);
  }
  else if (video_format.compare("NV12") == 0) {
    VLOG(DEBUG) << "Detected NV12 caps format=" << video_format;
    cv::Mat RGB = getRGBFrame(map, (gint)0);
    if (RGB.empty())
      LOG(FATAL) << "Could not unpack Left image";
    cv::imwrite(img_name.c_str(), RGB);
  }
  else if (video_format.compare("YV12") == 0) {
    /**
     * openCV type conversions
     * @reference:
     * https://docs.opencv.org/3.4/d8/d01/group__imgproc__color__conversions.html
     *
     */
    VLOG(DEBUG) << "Detected YV12 caps format=" << video_format;
    cv::Mat input_frame(height + height / 2, width, CV_8UC1, (char *)map.data, cv::Mat::AUTO_STEP);
    cv::Mat bgr_frame(width, height, CV_8UC3);
    cv::cvtColor(input_frame, bgr_frame, cv::COLOR_YUV2BGR_YV12, 3);
    cv::imwrite(img_name.c_str(), bgr_frame);
    cv::cvtColor(bgr_frame, input_frame, cv::COLOR_BGR2YUV_YV12, 3);
  }
  else {
    LOG(FATAL) << "Detected caps that we cannot convert for overlay writing:" << video_format;
  }

  // unref memory and exit
  gst_buffer_unmap(buffer, &map);
  return GST_PAD_PROBE_OK;
}

/**
 * @brief An EXAMPLE callback for nvidia element pad (sink of nvtracker) that extracts metadata
 *
 * @param pad               @description the pad to which the callback is attached
 * @param info              @description the data component of the gstreamer buffer
 * @param u_data            @description user data pointer passed into the callback
 * @return GstFlowReturn    @description return handle behaviour
 */
inline GstPadProbeReturn unpack_dsMeta_example(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  // map the Gstreamer
  GstBuffer *buf = (GstBuffer *)info->data;

  // extract NvDs batch meta
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
  if (!batch_meta) {
    LOG(ERROR) << "Batch meta not found for buffer " << buf;
    return GST_PAD_PROBE_OK;
  }

  // map the Nvidia buffer (surface) from GPU
  GstMapInfo in_map_info;
  NvBufSurface *surface = NULL;
  memset(&in_map_info, 0, sizeof(in_map_info));
  if (!gst_buffer_map(buf, &in_map_info, GST_MAP_READ)) {
    LOG(ERROR) << "Error: Failed to map gst buffer";
    gst_buffer_unmap(buf, &in_map_info);
    return GST_PAD_PROBE_OK;
  }

  // loop over sources
  NvDsMetaList *frame_list = NULL;
  NvDsMetaList *object_list = NULL;
  for (frame_list = batch_meta->frame_meta_list; frame_list != NULL; frame_list = frame_list->next) {
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(frame_list->data);

    // loop through detected objects
    int object_detected = 0;
    for (object_list = frame_meta->obj_meta_list; object_list != NULL; object_list = object_list->next) {
      // cast data so that it is accessible
      NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(object_list->data);

      // Unpack bounding box from metadata
      NvDsComp_BboxInfo tracker_bbox_info = obj_meta->tracker_bbox_info;
      NvBbox_Coords tracker_boxes = tracker_bbox_info.org_bbox_coords;

      /**
       * Add logic here
       */

    }  // loops through detected objects
  }    // loops through video sources

  gst_buffer_unmap(buf, &in_map_info);

  return GST_PAD_PROBE_OK;
}

}  // namespace GstCallbacks
}  // namespace core
