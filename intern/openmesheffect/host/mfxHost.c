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

/** \file
 * \ingroup openmesheffect
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "ofxProperty.h"
#include "ofxParam.h"
#include "ofxMeshEffect.h"

#include "util/ofx_util.h"
#include "util/memory_util.h"

#include "mfxHost.h"

// OFX SUITES MAIN

static const void *fetchSuite(OfxPropertySetHandle host, const char *suiteName, int suiteVersion)
{
  (void)host;  // TODO: check host?

  if (0 == strcmp(suiteName, kOfxMeshEffectSuite) && suiteVersion == 1) {
    switch (suiteVersion) {
      case 1:
        return &gMeshEffectSuiteV1;
      default:
        return NULL;
    }
  }
  if (0 == strcmp(suiteName, kOfxParameterSuite) && suiteVersion == 1) {
    switch (suiteVersion) {
      case 1:
        return &gParameterSuiteV1;
      default:
        return NULL;
    }
  }
  if (0 == strcmp(suiteName, kOfxPropertySuite) && suiteVersion == 1) {
    switch (suiteVersion) {
      case 1:
        return &gPropertySuiteV1;
      default:
        return NULL;
    }
  }

  return NULL;
}

// OFX MESH EFFECT HOST

OfxHost *gHost = NULL;
int gHostUse = 0;

OfxHost *getGlobalHost(void)
{
  if (0 == gHostUse) {
    gHost = malloc_array(sizeof(OfxHost), 1, "global host");
    OfxPropertySetHandle hostProperties = malloc_array(
        sizeof(OfxPropertySetStruct), 1, "global host properties");
    init_properties(hostProperties);
    hostProperties->context = PROP_CTX_HOST;
    propSetPointer(hostProperties, kOfxHostPropBeforeMeshReleaseCb, 0, (void *)NULL);
    propSetPointer(hostProperties, kOfxHostPropBeforeMeshGetCb, 0, (void *)NULL);
    gHost->host = hostProperties;
    gHost->fetchSuite = fetchSuite;
  }
  ++gHostUse;
  return gHost;
}

void releaseGlobalHost(void)
{
  if (--gHostUse == 0) {
    free_array(gHost->host);
    free_array(gHost);
    gHost = NULL;
  }
}

bool ofxhost_load_plugin(OfxHost *host, OfxPlugin *plugin)
{
  OfxStatus status;

  plugin->setHost(host);

  status = plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);

  if (kOfxStatReplyDefault == status) {
  }
  if (kOfxStatFailed == status) {
    return false;
  }
  if (kOfxStatErrFatal == status) {
    return false;
  }
  return true;
}

void ofxhost_unload_plugin(OfxPlugin *plugin)
{
  OfxStatus status;

  status = plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL);

  if (kOfxStatReplyDefault == status) {
  }
  if (kOfxStatErrFatal == status) {
  }

  plugin->setHost(NULL);
}

bool ofxhost_get_descriptor(OfxHost *host,
                            OfxPlugin *plugin,
                            OfxMeshEffectHandle *effectDescriptor)
{
  OfxStatus status;
  OfxMeshEffectHandle effectHandle;

  *effectDescriptor = NULL;
  effectHandle = malloc_array(sizeof(OfxMeshEffectStruct), 1, "mesh effect descriptor");

  effectHandle->host = host;
  init_mesh_effect(effectHandle);

  status = plugin->mainEntry(kOfxActionDescribe, effectHandle, NULL, NULL);

  if (kOfxStatErrMissingHostFeature == status) {
    return false;
  }
  if (kOfxStatErrMemory == status) {
    return false;
  }
  if (kOfxStatFailed == status) {
    return false;
  }
  if (kOfxStatErrFatal == status) {
    return false;
  }

  *effectDescriptor = effectHandle;

  return true;
}

void ofxhost_release_descriptor(OfxMeshEffectHandle effectDescriptor)
{
  free_mesh_effect(effectDescriptor);
  free_array(effectDescriptor);
}

bool ofxhost_create_instance(OfxPlugin *plugin,
                             OfxMeshEffectHandle effectDescriptor,
                             OfxMeshEffectHandle *effectInstance)
{
  OfxStatus status;
  OfxMeshEffectHandle instance;

  *effectInstance = NULL;

  instance = malloc_array(sizeof(OfxMeshEffectStruct), 1, "mesh effect descriptor");
  deep_copy_mesh_effect(instance, effectDescriptor);

  status = plugin->mainEntry(kOfxActionCreateInstance, instance, NULL, NULL);

  if (kOfxStatErrMemory == status) {
    return false;
  }
  if (kOfxStatFailed == status) {
    return false;
  }
  if (kOfxStatErrFatal == status) {
    return false;
  }

  *effectInstance = instance;

  return true;
}

void ofxhost_destroy_instance(OfxPlugin *plugin, OfxMeshEffectHandle effectInstance)
{
  OfxStatus status;

  status = plugin->mainEntry(kOfxActionDestroyInstance, effectInstance, NULL, NULL);

  if (kOfxStatFailed == status) {
  }
  if (kOfxStatErrFatal == status) {
  }

  free_mesh_effect(effectInstance);
  free_array(effectInstance);
}

bool ofxhost_cook(OfxPlugin *plugin, OfxMeshEffectHandle effectInstance)
{
  OfxStatus status;

  status = plugin->mainEntry(kOfxMeshEffectActionCook, effectInstance, NULL, NULL);

  if (kOfxStatErrMemory == status) {
    return false;
  }
  if (kOfxStatFailed == status) {
    return false;
  }
  if (kOfxStatErrFatal == status) {
    return false;
  }
  return true;
}

bool use_plugin(const PluginRegistry *registry, int plugin_index)
{
  OfxPlugin *plugin = registry->plugins[plugin_index];

  // Set host (TODO: do this in load_plugins?)
  OfxHost *host = getGlobalHost();

  // Load action if not loaded yet
  if (OfxPluginStatNotLoaded == registry->status[plugin_index]) {
    if (ofxhost_load_plugin(host, plugin)) {
      registry->status[plugin_index] = OfxPluginStatOK;
    }
    else {
      registry->status[plugin_index] = OfxPluginStatError;
      return false;
    }
  }

  if (OfxPluginStatError == registry->status[plugin_index]) {
    return false;
  }

  // Describe action
  OfxMeshEffectHandle effectDescriptor;
  if (ofxhost_get_descriptor(host, plugin, &effectDescriptor)) {
    OfxMeshEffectHandle effectInstance;

    // Create Instance action
    if (ofxhost_create_instance(plugin, effectDescriptor, &effectInstance)) {
      ofxhost_cook(plugin, effectInstance);
      ofxhost_destroy_instance(plugin, effectInstance);
    }
    ofxhost_release_descriptor(effectDescriptor);
  }

  // Unload action (TODO: move into e.g. free_registry)
  // ofxhost_unload_plugin(plugin);
  // releaseGlobalHost();

  return true;
}
