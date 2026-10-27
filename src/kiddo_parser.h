/**
 * @file kiddo_parser.h
 * @brief Kiddo markup language parser
 *
 * This file contains the KiddoParser namespace for parsing Kiddo markup
 * language into structured content segments with text and image support.
 */

#pragma once

#include <Arduino.h>
#include <vector>

namespace KiddoParser {

/**
 * @brief Structure representing a content segment
 */
struct ContentSegment {
    enum Type { TEXT, IMAGE };   /**< Type of content segment */
    Type type;                   /**< Segment type */
    String content;              /**< Segment content */

    /**
     * @brief Constructor
     * @param t Segment type
     * @param c Segment content
     */
    ContentSegment(Type t, const String& c) : type(t), content(c) {}
};

/**
 * @brief Structure representing parsed content
 */
struct ParsedContent {
    std::vector<ContentSegment> segments;  /**< Parsed content segments */
    String plain_text;                     /**< Plain text version */
};

/**
 * @brief Parse text content with image references
 * @param input Input text to parse
 * @return ParsedContent structure with segments and plain text
 */
ParsedContent parseText(const String& input);

/**
 * @brief Extract image URLs from text content
 * @param input Input text to scan for image URLs
 * @return Vector of image URLs found
 */
std::vector<String> getImageUrls(const String& input);

}
