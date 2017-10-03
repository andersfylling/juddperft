project (juddperft)

# Files to be compiled.
file(GLOB juddperft_files
  # main
  Juddperft.cpp

  movegen.cpp
  search.cpp
  hashtable.cpp
  fen.cpp
  engine.cpp
  diagnostics.cpp
  timemanage.cpp
  winboard.cpp
)
add_executable(juddperft ${juddperft_files})

target_compile_definitions(juddperft PUBLIC JUDDPERFT_PRODUCTION)

# Make this folder linkable for parent folder.
target_link_libraries(juddperft -latomic -pthread)
