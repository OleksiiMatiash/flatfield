#pragma once
#include "exiv2/exiv2.hpp"
#include "DataStructs.h"

class MetadataReader
{
	enum ExifTagsEnum
	{
		ImageWidth = 0x0100,
		ImageHeight = 0x0101,
		Compression = 0x0103,
		PhotometricInterpretation = 0x0106,
		CameraMaker = 0x010f,
		CameraModel = 0x0110,
		StripOffsets = 0x0111,
		SamplesPerPixel = 0x0115,
		StripByteCounts = 0x0117,
		CFAPattern2 = 0x828e,
		FNumber = 0x829d,
		FocalLength = 0x920a,
		LensModel = 0xa434,
		BlackLevel = 0xc61a,
		WhiteLevel = 0xc61d,
		ActiveArea = 0xc68d
	};

	static int findRawIFD(Exiv2::ExifData* exifData, int* photometricInterpretation);

public:
	static QSharedPointer<Metadata> readMetadata(const QString& filename);
};