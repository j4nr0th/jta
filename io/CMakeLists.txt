add_executable(jio_test_csv iobase.c iocsv.c ioerr.c tests/csv_test.c)
configure_file(tests/simple.csv "${CMAKE_BINARY_DIR}/csv_test_simple.csv" COPYONLY)
target_link_libraries(jio_test_csv PRIVATE jmem jdm)
add_test(NAME csv_test COMMAND jio_test_csv)
