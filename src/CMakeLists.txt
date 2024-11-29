# VTK'nin kurulum dizinini burada belirtin
set(VTK_DIR "/home/inferna/vtk/build")

# VTK'yi bul
find_package(VTK REQUIRED)

# Dahil edilen dosyalar ve kütüphane yollarını ekle
include(${VTK_USE_FILE})

# Kaynağı bu projenin yürütülebilir dosyasına ekleyin.
add_library(OptomoVTK SHARED 
    "OptomoVTK.cpp"
    "OptomoVTK.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OptomoVTK PROPERTY CXX_STANDARD 20)
endif()

# Platforma özgü ayarlar
if(WIN32)
    # Windows için DLL export ve import ayarları
    target_compile_definitions(OptomoVTK PRIVATE "MODEL_EXPORTS")
    set_target_properties(OptomoVTK PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)
elseif(UNIX)
    # Linux için herhangi bir platforma özgü ayar gerekmez.
    set_target_properties(OptomoVTK PROPERTIES PREFIX "lib")
endif()

# VTK kütüphanelerini bağla
target_link_libraries(OptomoVTK PRIVATE ${VTK_LIBRARIES})

# VTK include dizinlerini ekle
target_include_directories(OptomoVTK PRIVATE ${VTK_INCLUDE_DIRS})
