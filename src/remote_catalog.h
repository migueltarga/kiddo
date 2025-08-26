#pragma once
#include <Arduino.h>
#include <vector>

namespace remote_catalog {
  struct Entry { String file; String name; String lang; };

  String getCatalogUrl();

  void setCatalogUrl(const String& url);

  bool fetch();

  const std::vector<Entry>& entries();

  bool last_ok();

  void invalidate();

  int reconcileExisting();

  bool ensureDownloadedOrIndexed(const String& file, String* outStoryId = nullptr);
  
  int clearDownloads();
}
