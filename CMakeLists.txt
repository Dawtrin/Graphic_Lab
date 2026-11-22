cmake_minimum_required(VERSION 3.15)
project(OpenGL_GLUT_Lab CXX)

# Thiết lập chuẩn C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Xuất file compile_commands.json hỗ trợ IntelliSense trong VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tự động tìm kiếm các gói OpenGL và GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Thêm đường dẫn include nếu có
if(GLUT_INCLUDE_DIRS)
    include_directories(${GLUT_INCLUDE_DIRS})
endif()
if(OPENGL_INCLUDE_DIRS)
    include_directories(${OPENGL_INCLUDE_DIRS})
endif()

# Quét tất cả các file mã nguồn .cpp nằm trong mọi thư mục con (Lab_1, Lab_2, ...)
# CONFIGURE_DEPENDS giúp CMake tự động cập nhật danh sách file mới khi build
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Duyệt qua từng file nguồn để tạo target thực thi độc lập dựa trên tên file (NAME_WE)
foreach(SOURCE_FILE ${ALL_SOURCES})
    # Bỏ qua các file tạm sinh ra trong thư mục build/
    if(SOURCE_FILE MATCHES "CMakeFiles" OR SOURCE_FILE MATCHES "[/\\\\]build[/\\\\]")
        continue()
    endif()

    get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
    get_filename_component(PARENT_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(DIR_NAME ${PARENT_DIR} NAME)
    
    # Thay thế các ký tự không hợp lệ như ( ) bằng _
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" SAFE_DIR_NAME "${DIR_NAME}")
    
    # Kết hợp tên thư mục và tên file để tạo target duy nhất (VD: Lab_Graphic_ToMau_main)
    set(TARGET_NAME "${SAFE_DIR_NAME}_${FILE_NAME}")
    
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    # Liên kết đầy đủ các thư viện cần thiết
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${GLUT_LIBRARIES}
        ${OPENGL_LIBRARIES}
        glu32
        opengl32
        freeglut
    )
endforeach()
