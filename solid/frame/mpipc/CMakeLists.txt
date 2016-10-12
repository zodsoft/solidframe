
set(Sources
	src/mpipcerror.cpp
	src/mpipcservice.cpp
	src/mpipclistener.cpp
	src/mpipcconnection.cpp
	src/mpipcconfiguration.cpp
	src/mpipcmessagereader.cpp
	src/mpipcmessagewriter.cpp
	src/mpipcprotocol.cpp
)

set(Headers
	mpipcconfiguration.hpp
	mpipccontext.hpp
	mpipcerror.hpp
	mpipcmessage.hpp
	mpipcprotocol.hpp
	mpipcprotocol_serialization_v1.hpp
	mpipcservice.hpp
	mpipcsocketstub.hpp
	mpipcsocketstub_openssl.hpp
	mpipcsocketstub_plain.hpp
)

set(Inlines
)

add_library (solid_frame_mpipc
	${Sources} ${Headers} ${Inlines}
)

install (FILES ${Headers} ${Inlines} DESTINATION include/solid/frame/mpipc)
install (TARGETS solid_frame_mpipc DESTINATION lib)

if(NOT ON_CROSS)
	add_subdirectory(test)
endif()