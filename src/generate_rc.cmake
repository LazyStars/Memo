function(generate_rc rc_out OUTPUT_DIR)
    set (options)
    set (oneValueArgs
            NAME
            ICON
            ICON_PATH
            VERSION
            COMPANY_NAME
            COMPANY_COPYRIGHT
            COMMENTS
            ORIGINAL_FILENAME
            INTERNAL_NAME
            FILE_DESCRIPTION
            TRADEMARKS)
    set (multiValueArgs)

    cmake_parse_arguments(PRODUCT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target ${PRODUCT_NAME})

    set(target_version ${PRODUCT_VERSION})

    if(CMAKE_GENERATOR_PLATFORM MATCHES "86" OR CMAKE_GENERATOR_PLATFORM MATCHES "32")
        set(TARGET_ARCH "x86")
    else(CMAKE_GENERATOR_PLATFORM MATCHES "64")
        set(TARGET_ARCH "x64")
    endif()

    # 获取输出目录参数
    set(output_dir "${OUTPUT_DIR}")
    file(MAKE_DIRECTORY "${output_dir}")
    set(rc_file_output "${output_dir}/${target}_resource.rc")

    set(company_name "")
    if (DEFINED PRODUCT_COMPANY_NAME)
        set(company_name "${PRODUCT_COMPANY_NAME}")
    endif()

    set(file_description "${PRODUCT_NAME} (${TARGET_ARCH})")
    if (DEFINED PRODUCT_FILE_DESCRIPTION)
        set(file_description "${PRODUCT_FILE_DESCRIPTION} (${TARGET_ARCH})")
    endif()

    string(TIMESTAMP CURRENT_YEAR "%Y")
    set(legal_copyright "Copyright \\xA9 ${CURRENT_YEAR} ${PRODUCT_COMPANY_NAME}")
    if (DEFINED PRODUCT_COMPANY_COPYRIGHT)
        set(legal_copyright "${PRODUCT_COMPANY_COPYRIGHT}")
    endif()

    set(product_name "")
    if (DEFINED PRODUCT_PRODUCT_NAME)
        set(product_name "${PRODUCT_PRODUCT_NAME}")
    else()
        set(product_name "${PRODUCT_NAME}")
    endif()

    set(comments "${PRODUCT_NAME} v${PRODUCT_VERSION}")
    if (DEFINED PRODUCT_COMMENTS)
        set(comments "${PRODUCT_COMMENTS}")
    endif()

    set(legal_trademarks "")
    if (DEFINED PRODUCT_TRADEMARKS)
        set(legal_trademarks "${PRODUCT_TRADEMARKS}")
    endif()

    set(product_version "")
    if (target_version)
        if(target_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            # nothing to do
        elseif(target_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(target_version "${target_version}.0")
        elseif(target_version MATCHES "[0-9]+\\.[0-9]+")
            set(target_version "${target_version}.0.0")
        elseif (target_version MATCHES "[0-9]+")
            set(target_version "${target_version}.0.0.0")
        else()
            message(FATAL_ERROR "Invalid version format: '${target_version}'")
        endif()
        set(product_version "${target_version}")
    else()
        set(product_version "0.0.0.0")
    endif()

    set(file_version "${product_version}")
    string(REPLACE "src" "," version_comma ${product_version})

    set(original_file_name "${target}")
    if (PRODUCT_ORIGINAL_FILENAME)
        set(original_file_name "${PRODUCT_ORIGINAL_FILENAME}")
    endif()

    set(internal_name "${PRODUCT_NAME}")
    if (INTERNAL_NAME)
        set(internal_name "${INTERNAL_NAME}")
    endif()

    #IDI_ICON1               ICON                    \"logo.ico\"
    # 处理 logo.ico 路径
    set(logo_icon_line "")
    if(PRODUCT_ICON_PATH)
        set(logo_icon_line "IDI_ICON1               ICON                    \"${PRODUCT_ICON_PATH}\"")
    endif()

    set(icons "")
    if (PRODUCT_ICON)
        set(index 1)
        foreach(icon IN LISTS PRODUCT_ICON)
            string(APPEND icons "IDI_ICON${index}    ICON    \"${icon}\"\n")
            math(EXPR index "${index} +1")
        endforeach()
    endif()

    set(target_file_type "VFT_DLL")
    if(target_type STREQUAL "EXECUTABLE")
        set(target_file_type "VFT_APP")
    endif()

    set(contents
            "#include <windows.h>

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_CHS)
LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED
#pragma code_page(936)
#endif

${logo_icon_line}

//${icons}
VS_VERSION_INFO VERSIONINFO
FILEVERSION ${version_comma}
PRODUCTVERSION ${version_comma}
FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
    FILEFLAGS VS_FF_DEBUG
#else
    FILEFLAGS 0x0L
#endif
FILEOS VOS_NT_WINDOWS32
FILETYPE ${target_file_type}
FILESUBTYPE VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
        //公司名称、文件描述、文件版本、法律版权、原始文件名、产品名称、产品版本、评论、合法商标、内部名
            VALUE \"CompanyName\", \"${company_name}\"
            VALUE \"FileDescription\", \"${file_description}\"
            VALUE \"FileVersion\", \"${file_version}\"
            VALUE \"LegalCopyright\", \"${legal_copyright}\"
            VALUE \"OriginalFilename\", \"${original_file_name}\"
            VALUE \"ProductName\", \"${product_name}\"
            VALUE \"ProductVersion\", \"${product_version}\"
            VALUE \"Comments\", \"${comments}\"
            VALUE \"LegalTrademarks\", \"${legal_trademarks}\"
            VALUE \"InternalName\", \"${internal_name}\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x0409, 1200
    END
END
/* End of Version info */\n"
    )

    file(WRITE "${rc_file_output}" "${contents}")
    set(${rc_out} ${rc_file_output} PARENT_SCOPE)

endfunction(generate_rc)
