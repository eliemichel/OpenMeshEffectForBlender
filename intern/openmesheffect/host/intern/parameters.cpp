/*
 * Copyright 2019 Elie Michel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parameters.h"
#include "properties.h"

#include "util/memory_util.h"

#include "ofxParam.h"

#include <string.h>
#include <stdio.h>

// OFX PARAMETERS SUITE

// // OfxParamStruct

OfxParamStruct::OfxParamStruct()
{
  type = PARAM_TYPE_DOUBLE;
  name = NULL;
  init_properties(&properties);
  properties.context = PROP_CTX_PARAM;
}

OfxParamStruct::~OfxParamStruct()
{
  free_properties(&properties);
  if (PARAM_TYPE_STRING == type) {
    free_array(value[0].as_char);
  }
  if (NULL != name) {
    free_array(name);
  }
}

void OfxParamStruct::set_type(ParamType type)
{
  if (this->type == type) {
    return;
  }

  if (PARAM_TYPE_STRING == this->type) {
    free_array(this->value[0].as_char);
    this->value[0].as_char = NULL;
  }

  this->type = type;

  if (PARAM_TYPE_STRING == this->type) {
    this->value[0].as_char = NULL;
    realloc_string(1);
  }
}

void OfxParamStruct::realloc_string(int size)
{
  if (NULL != this->value[0].as_char) {
    free_array(this->value[0].as_char);
  }
  this->value[0].as_char = (char *)malloc_array(sizeof(char), size, "parameter string value");
  this->value[0].as_char[0] = '\0';
}

void OfxParamStruct::deep_copy_from(const OfxParamStruct &other)
{
  this->name = other.name;
  if (NULL != this->name) {
    this->name = (char *)malloc_array(sizeof(char), strlen(other.name) + 1, "parameter name");
    strcpy(this->name, other.name);
  }
  this->type = other.type;
  this->value[0] = other.value[0];
  this->value[1] = other.value[1];
  this->value[2] = other.value[2];
  this->value[3] = other.value[3];

  // Strings are dynamically allocated, so deep copy must allocate new data
  if (this->type == PARAM_TYPE_STRING) {
    int n = strlen(other.value[0].as_char);
    this->value[0].as_char = NULL;
    realloc_string(n + 1);
    strcpy(this->value[0].as_char, other.value[0].as_char);
  }

  deep_copy_property_set(&this->properties, &other.properties);
}

// // OfxParamSetStruct

OfxParamSetStruct::OfxParamSetStruct()
{
  num_parameters = 0;
  parameters = NULL;
  effect_properties = NULL;
}

OfxParamSetStruct::~OfxParamSetStruct()
{
  for (int i = 0; i < num_parameters; ++i) {
    delete parameters[i];
  }
  num_parameters = 0;
  if (NULL != parameters) {
    free_array(parameters);
    parameters = NULL;
  }
}

int OfxParamSetStruct::find_parameter(const char *param)
{
  for (int i = 0 ; i < this->num_parameters ; ++i) {
    if (0 == strcmp(this->parameters[i]->name, param)) {
      return i;
    }
  }
  return -1;
}

void OfxParamSetStruct::append_parameters(int count)
{
  int old_num_parameters = this->num_parameters;
  OfxParamStruct **old_parameters = this->parameters;
  this->num_parameters += count;
  this->parameters = (OfxParamStruct **)malloc_array(
      sizeof(OfxParamStruct *), this->num_parameters, "parameters");
  for (int i = 0; i < this->num_parameters; ++i) {
    if (i < old_num_parameters) {
      this->parameters[i] = old_parameters[i];
    } else {
      this->parameters[i] = new OfxParamStruct();
    }
  }
  if (NULL != old_parameters) {
    free_array(old_parameters);
  }
}

int OfxParamSetStruct::ensure_parameter(const char *parameter)
{
  int i = find_parameter(parameter);
  if (i == -1) {
    append_parameters(1);
    i = this->num_parameters - 1;
    this->parameters[i]->name = (char *)malloc_array(
        sizeof(char), strlen(parameter) + 1, "parameter name");
    strcpy(this->parameters[i]->name, parameter);
  }
  return i;
}

void OfxParamSetStruct::deep_copy_from(const OfxParamSetStruct &other)
{
  append_parameters(other.num_parameters);
  for (int i = 0 ; i < this->num_parameters ; ++i) {
    this->parameters[i]->deep_copy_from(*other.parameters[i]);
  }
  this->effect_properties = other.effect_properties;
}

// // Utils

ParamType parse_parameter_type(const char *str) {
  if (0 == strcmp(str, kOfxParamTypeInteger)) {
    return PARAM_TYPE_INTEGER;
  }
  if (0 == strcmp(str, kOfxParamTypeInteger2D)) {
    return PARAM_TYPE_INTEGER_2D;
  }
  if (0 == strcmp(str, kOfxParamTypeInteger3D)) {
    return PARAM_TYPE_INTEGER_3D;
  }
  if (0 == strcmp(str, kOfxParamTypeDouble)) {
    return PARAM_TYPE_DOUBLE;
  }
  if (0 == strcmp(str, kOfxParamTypeDouble2D)) {
    return PARAM_TYPE_DOUBLE_2D;
  }
  if (0 == strcmp(str, kOfxParamTypeDouble3D)) {
    return PARAM_TYPE_DOUBLE_3D;
  }
  if (0 == strcmp(str, kOfxParamTypeRGB)) {
    return PARAM_TYPE_RGB;
  }
  if (0 == strcmp(str, kOfxParamTypeRGBA)) {
    return PARAM_TYPE_RGBA;
  }
  if (0 == strcmp(str, kOfxParamTypeBoolean)) {
    return PARAM_TYPE_BOOLEAN;
  }
  if (0 == strcmp(str, kOfxParamTypeChoice)) {
    return PARAM_TYPE_CHOICE;
  }
  if (0 == strcmp(str, kOfxParamTypeString)) {
    return PARAM_TYPE_STRING;
  }
  if (0 == strcmp(str, kOfxParamTypeCustom)) {
    return PARAM_TYPE_CUSTOM;
  }
  if (0 == strcmp(str, kOfxParamTypePushButton)) {
    return PARAM_TYPE_PUSH_BUTTON;
  }
  if (0 == strcmp(str, kOfxParamTypeGroup)) {
    return PARAM_TYPE_GROUP;
  }
  if (0 == strcmp(str, kOfxParamTypePage)) {
    return PARAM_TYPE_PAGE;
  }
  return PARAM_TYPE_UNKNOWN;
}

size_t parameter_type_dimensions(ParamType type) {
  switch (type) {
  case PARAM_TYPE_INTEGER:
  case PARAM_TYPE_DOUBLE:
  case PARAM_TYPE_BOOLEAN:
  case PARAM_TYPE_STRING:
    return 1;
  case PARAM_TYPE_INTEGER_2D:
  case PARAM_TYPE_DOUBLE_2D:
    return 2;
  case PARAM_TYPE_INTEGER_3D:
  case PARAM_TYPE_DOUBLE_3D:
  case PARAM_TYPE_RGB:
    return 3;
  case PARAM_TYPE_RGBA:
    return 4;
  default:
    return 1;
  }
}
