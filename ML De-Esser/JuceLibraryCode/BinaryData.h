/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   two_model_json;
    const int            two_model_jsonSize = 8765472;

    extern const char*   three_model_json;
    const int            three_model_jsonSize = 8765053;

    extern const char*   five_model_json;
    const int            five_model_jsonSize = 8680122;

    extern const char*   nine_model_json;
    const int            nine_model_jsonSize = 8733404;

    extern const char*   all_model_json;
    const int            all_model_jsonSize = 8919468;

    extern const char*   csd_low_model_json;
    const int            csd_low_model_jsonSize = 8808000;

    extern const char*   csd_high_model_json;
    const int            csd_high_model_jsonSize = 8984885;

    extern const char*   model_json;
    const int            model_jsonSize = 6461894;

    extern const char*   model_csd_json;
    const int            model_csd_jsonSize = 6461869;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 9;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
