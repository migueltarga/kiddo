#include "kiddo_parser.h"

namespace KiddoParser {

ParsedContent parseText(const String& input) {
    ParsedContent result;
    String working = input;
    String plain_text = "";
    
    int pos = 0;
    while (pos < working.length()) {
        // Look for the next [img] tag
        int img_start = working.indexOf("[img]", pos);
        
        if (img_start == -1) {
            // No more images, add remaining text
            String remaining_text = working.substring(pos);
            if (remaining_text.length() > 0) {
                result.segments.push_back(ContentSegment(ContentSegment::TEXT, remaining_text));
                plain_text += remaining_text;
            }
            break;
        }
        
        // Add text before the image (if any)
        if (img_start > pos) {
            String text_before = working.substring(pos, img_start);
            result.segments.push_back(ContentSegment(ContentSegment::TEXT, text_before));
            plain_text += text_before;
        }
        
        // Find the closing [/img] tag
        int img_end = working.indexOf("[/img]", img_start + 5);
        if (img_end == -1) {
            // No closing tag, treat as regular text
            String remaining_text = working.substring(img_start);
            result.segments.push_back(ContentSegment(ContentSegment::TEXT, remaining_text));
            plain_text += remaining_text;
            break;
        }
        
        // Extract the image URL
        String url = working.substring(img_start + 5, img_end);
        if (url.length() > 0) {
            result.segments.push_back(ContentSegment(ContentSegment::IMAGE, url));
        }
        
        // Continue after the [/img] tag
        pos = img_end + 6;
    }
    
    result.plain_text = plain_text;
    return result;
}

std::vector<String> getImageUrls(const String& input) {
    std::vector<String> urls;
    String working = input;
    int pos = 0;
    
    while (true) {
        int img_start = working.indexOf("[img]", pos);
        if (img_start == -1) break;
        
        int img_end = working.indexOf("[/img]", img_start + 5);
        if (img_end == -1) break;
        
        String url = working.substring(img_start + 5, img_end);
        if (url.length() > 0) {
            urls.push_back(url);
        }
        
        pos = img_end + 6;
    }
    
    return urls;
}

}
