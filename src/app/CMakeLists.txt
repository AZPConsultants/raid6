add_executable(raid6
               raid6.cpp)
			   
find_package(Boost COMPONENTS iostreams REQUIRED)
find_package(docopt CONFIG REQUIRED)
find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(raid6 PRIVATE 
							libraid6
							docopt
							Boost::iostreams
							fmt::fmt
							${Boost_LIBRARIES}
							${LIBMONGOCXX_LIBRARIES}
							${LIBBSONCXX_LIBRARIES})

