#pragma once

#include <regex>
#include <string>

#include "logging.hpp"


namespace yamlParser {

/**
 * @brief check if a string is a float number
 * @param str the input string
 * @return `bool` true if it is a float
 */
inline bool isFloat(const std::string &str)
{
  std::regex pattern("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$");
  return std::regex_match(str, pattern);
}

/**
 * @brief check if a string is an integer number
 * @param str the input string
 * @return `bool` true if it is an integer
 */
inline bool isInt(const std::string &str)
{
  std::regex pattern("^[0-9]+$");
  return std::regex_match(str, pattern);
}

/**
 * @brief link gstreamer elements that have static pads
 * @param pipeline pipeline
 * @param new_element the new element that was just created in the YAML file
 * @param name the name name of this element from the YAML file
 * @param last_element the alias name of the previous element from the YAML file
 * @return
 */
inline bool link_static_pad_elements(GstElement *pipeline, GstElement *new_element, std::string name, std::string last_element)
{
  if (last_element.empty()) {
    LOG(ERROR) << "[BUG] The previous element is empty and there is not link field!";
    return false;
  }

  VLOG(DEBUG) << "Static linking elements (src= " << last_element << ": sink=" << name << ")";
  GstElement *src = (GstElement *)gst_bin_get_by_name(GST_BIN(pipeline), last_element.c_str());
  if (!gst_element_link(src, new_element)) {
    LOG(ERROR) << "failed to static link elements (src=" << GST_ELEMENT_NAME(src) << ",sink=" << GST_ELEMENT_NAME(new_element) << ")";
    return false;
  }
  return true;
}

/**
 * @brief sets element property field by parsing through `property` key list in YAML file
 * @param property_key the `property` key in the YAML file
 * @return true if successful, otherwise false
 */
inline bool set_element_properties(GstElement *new_element, const YAML::Node &property_key)
{
  for (const auto &property : property_key) {
    std::string prop;
    try {
      prop = property.as<std::string>();
    }
    catch (const std::exception &e) {
      LOG(ERROR) << "ERROR getting YAML field from property=" << property << ": ErrMsg=" << e.what();
      return false;
    }
    // Split the fruit item by "=" sign
    std::istringstream iss(prop);
    std::string key, value;
    std::getline(iss, key, '=');
    std::getline(iss, value);

    // set element property based on its type
    if (isInt(value)) {
      VLOG(DEBUG) << "\t\tprop[int]: " << key << "," << value;
      g_object_set(G_OBJECT(new_element), key.c_str(), std::stoi(value), NULL);
    }
    else if (key == "caps") {
      VLOG(DEBUG) << "\t\tprop[caps]: " << key << "," << value;
      g_object_set(G_OBJECT(new_element), key.c_str(), gst_caps_from_string(value.c_str()), NULL);
    }
    else if (value == "true") {
      VLOG(DEBUG) << "\t\tprop[true]: " << key << "," << true;
      g_object_set(G_OBJECT(new_element), key.c_str(), true, NULL);
    }
    else if (value == "false") {
      VLOG(DEBUG) << "\t\tprop[false]: " << key << "," << false;
      g_object_set(G_OBJECT(new_element), key.c_str(), false, NULL);
    }
    else if (isFloat(value)) {
      VLOG(DEBUG) << "\t\tprop[float]: " << key << "," << value;
      g_object_set(G_OBJECT(new_element), key.c_str(), std::stof(value), NULL);
    }
    else {
      VLOG(DEBUG) << "\t\tprop[str]: " << key << "," << value;
      g_object_set(G_OBJECT(new_element), key.c_str(), value.c_str(), NULL);
    }
  }
  return true;
}

/**
 * @brief set the links between elements
 * @param pipeline the gstreamer pipeline
 * @param new_element the new element that was just created
 * @param element the YAML element that has been parsed
 * @param name the name of the element from the YAML file
 * @return
 */
inline bool set_links(GstElement *pipeline, GstElement *new_element, YAML::Node element, std::string name)
{
  std::string link_type = element["link"]["type"].as<std::string>();
  if (link_type == "request") {
    std::string pad_name = element["link"]["pad_name"].as<std::string>();
    std::string link_element = element["link"]["link_element"].as<std::string>();
    std::string link_pad = element["link"]["link_pad"].as<std::string>();
    VLOG(DEBUG) << "\t link_type " << link_type << ", pad_name=" << pad_name << ",link_element=" << link_element << ",link_pad=" << link_pad;

    GstElement *src_element = (GstElement *)gst_bin_get_by_name(GST_BIN(pipeline), link_element.c_str());

    GstPad *src_pad = gst_element_get_static_pad(src_element, link_pad.c_str());
    GstPad *sink_pad = gst_element_get_request_pad(new_element, pad_name.c_str());
    int ret = gst_pad_link(src_pad, sink_pad);
    // unref objects and check if pads linked okay
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
    // check that link was successful
    if (ret != GST_PAD_LINK_OK) {
      LOG(ERROR) << "LINK ERROR [" << name << "] link to element=" << link_element.c_str() << " :pad=" << link_pad << "::  ErrMsg=" << get_link_status(ret);
      return false;
    }
  }
  else if (link_type == "ignore") {
    VLOG(DEBUG) << "Ignoring link and assuming you have a sometimes or request to handle it";
  }
  else {
    LOG(WARNING) << "[BUG] Link logic error";
    return false;
  }
  return true;
}

inline bool link_pipeline_elements(GstElement *pipeline, GstElement *new_element, std::string last_element, YAML::Node element, YAML::Node config)
{
  std::string name;
  try {
    name = element["alias"].as<std::string>();
  }
  catch (const std::exception &e) {
    LOG(ERROR) << "ERROR getting YAML field from 'element.alias' or ErrMsg=" << e.what();
    return false;
  }

  // link element in pipeline
  if (element["link"]) {
    if (!set_links(pipeline, new_element, element, name))
      return false;
  }
  else if (name == "source") {
    // link to hls branch if this is the source
    // continue
  }
  else {
    if (!link_static_pad_elements(pipeline, new_element, name, last_element))
      return false;
  }
  return true;
}

} // namespace yamlParser