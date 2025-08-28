#ifndef SHADER_PARSER_H
#define SHADER_PARSER_H

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INCLUDE_DEPTH 10
#define MAX_LINE_LENGTH 1024
#define MAX_SHADER_SIZE (1024 * 1024) // 1MB max shader size

// Simple header-only include processor for GLSL shaders
static char* shader_process_includes(const char* filepath, int depth)
{
    if(depth > MAX_INCLUDE_DEPTH) {
        TraceLog(LOG_ERROR, "SHADER_INCLUDE: Include depth exceeded in %s", filepath);
        return NULL;
    }

    char* source = LoadFileText(filepath);
    if(!source) {
        TraceLog(LOG_ERROR, "SHADER_INCLUDE: Failed to load %s", filepath);
        return NULL;
    }

    // Allocate buffer for processed shader
    char* processed = malloc(MAX_SHADER_SIZE);
    if(!processed) {
        TraceLog(LOG_ERROR, "SHADER_INCLUDE: Memory allocation failed");
        UnloadFileText(source);
        return NULL;
    }
    processed[0] = '\0';

    char   line[MAX_LINE_LENGTH];
    char*  src_ptr       = source;
    size_t processed_len = 0;

    // Process line by line
    while(*src_ptr && processed_len < MAX_SHADER_SIZE - 1024) {
        // Extract line
        char*  line_end = strchr(src_ptr, '\n');
        size_t line_len;

        if(line_end) {
            line_len = line_end - src_ptr;
        } else {
            line_len = strlen(src_ptr);
        }

        if(line_len >= MAX_LINE_LENGTH - 1)
            line_len = MAX_LINE_LENGTH - 1;

        strncpy(line, src_ptr, line_len);
        line[line_len] = '\0';

        // Trim whitespace from start of line
        char* trimmed = line;
        while(*trimmed == ' ' || *trimmed == '\t')
            trimmed++;

        // Check for #include directive
        if(strncmp(trimmed, "#include", 8) == 0) {
            // Extract filename from #include "filename" or #include <filename>
            char* quote1   = strchr(trimmed, '"');
            char* bracket1 = strchr(trimmed, '<');
            char  end_char = '"';

            if(bracket1 && (!quote1 || bracket1 < quote1)) {
                quote1   = bracket1;
                end_char = '>';
            }

            if(quote1) {
                quote1++; // Skip opening quote/bracket
                char* quote2 = strchr(quote1, end_char);
                if(quote2) {
                    *quote2 = '\0'; // Null terminate filename

                    // Build include path
                    char include_path[512];
                    if(quote1[0] == '/' || (quote1[1] == ':' && quote1[2] == '\\')) {
                        // Absolute path
                        snprintf(include_path, sizeof(include_path), "%s", quote1);
                    } else {
                        // Relative path - assume shaders/ directory
                        snprintf(include_path, sizeof(include_path), "shaders/%s", quote1);
                    }

                    // Add comment showing what's being included
                    char include_comment[256];
                    snprintf(include_comment, sizeof(include_comment), "// BEGIN INCLUDE: %s\n", quote1);
                    strcat(processed, include_comment);
                    processed_len += strlen(include_comment);

                    // Recursively process included file
                    char* included_content = shader_process_includes(include_path, depth + 1);
                    if(included_content) {
                        size_t included_len = strlen(included_content);
                        if(processed_len + included_len < MAX_SHADER_SIZE - 512) {
                            strcat(processed, included_content);
                            processed_len += included_len;
                        } else {
                            TraceLog(LOG_ERROR, "SHADER_INCLUDE: Shader too large after including %s", quote1);
                        }
                        free(included_content);
                    } else {
                        TraceLog(LOG_WARNING, "SHADER_INCLUDE: Failed to include %s", quote1);
                        // Add original line as comment so compilation shows the error
                        char error_comment[256];
                        snprintf(error_comment, sizeof(error_comment), "// ERROR: %.240s\n", line);
                        strcat(processed, error_comment);
                        processed_len += strlen(error_comment);
                    }

                    // Add end comment
                    snprintf(include_comment, sizeof(include_comment), "// END INCLUDE: %s\n", quote1);
                    strcat(processed, include_comment);
                    processed_len += strlen(include_comment);
                } else {
                    // Malformed include, keep original line
                    strcat(processed, line);
                    strcat(processed, "\n");
                    processed_len += line_len + 1;
                }
            } else {
                // No quotes found, keep original line
                strcat(processed, line);
                strcat(processed, "\n");
                processed_len += line_len + 1;
            }
        } else {
            // Regular line, copy as-is
            if(processed_len + line_len + 1 < MAX_SHADER_SIZE - 1) {
                strcat(processed, line);
                strcat(processed, "\n");
                processed_len += line_len + 1;
            }
        }

        // Move to next line
        src_ptr += line_len;
        if(line_end && *src_ptr == '\n')
            src_ptr++;
    }

    UnloadFileText(source);

    // Shrink buffer to actual size
    char* final_processed = malloc(strlen(processed) + 1);
    if(final_processed) {
        strcpy(final_processed, processed);
    }
    free(processed);

    return final_processed;
}

static inline char* LoadShaderWithIncludes(const char* filepath) { return shader_process_includes(filepath, 0); }

#endif // SHADER_PARSER_H
