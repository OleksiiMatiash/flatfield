#include "MetadataReader.h"

QSharedPointer<Metadata> MetadataReader::readMetadata(const QString& filename)
{
	const Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(filename.toStdString(), false);
	image->readMetadata();
	Exiv2::ExifData exifData = image->exifData();
	QSharedPointer<Metadata> metadata = QSharedPointer<Metadata>(new Metadata());
	int photometricInterpretation = 0;
	const int rawIFDid = findRawIFD(&exifData, &photometricInterpretation);

	for (Exiv2::ExifData::const_iterator iterator = exifData.begin(); iterator != exifData.end(); ++iterator)
	{
		switch (iterator->tag())
		{
			case ExifTagsEnum::CameraMaker:
				metadata->cameraMaker = QString::fromStdString(iterator->toString());
				break;
			case ExifTagsEnum::CameraModel:
				metadata->cameraModel = QString::fromStdString(iterator->toString());
				break;
			case ExifTagsEnum::FNumber:
				metadata->fNumber = iterator->toFloat();
				break;
			case ExifTagsEnum::FocalLength:
				metadata->focalLength = iterator->toFloat();
				break;
			case ExifTagsEnum::LensModel:
				metadata->lens = QString::fromStdString(iterator->toString());
				break;
		}

		if (static_cast<int>(iterator->ifdId()) == rawIFDid)
		{
			switch (iterator->tag())
			{
				case ExifTagsEnum::ImageWidth:
					metadata->imageWidth = static_cast<int>(iterator->toInt64());
					break;
				case ExifTagsEnum::ImageHeight:
					metadata->imageHeight = static_cast<int>(iterator->toInt64());
					break;
				case ExifTagsEnum::Compression:
					if (iterator->toUint32() != 1)
					{
						return nullptr;
					}
					break;
				case ExifTagsEnum::StripOffsets:
					metadata->dataOffset = static_cast<int>(iterator->toInt64(0));
					break;
				case ExifTagsEnum::SamplesPerPixel:
					switch (photometricInterpretation)
					{
						case 34892:
							switch (iterator->toUint32())
							{
								case 1:
									metadata->rawType = Metadata::RawTypeEnum::Mono;
									break;
								case 3:
									metadata->rawType = Metadata::RawTypeEnum::RGB;
									break;
								default:
									return nullptr;
							}
							break;
						case 32803:
							metadata->rawType = Metadata::RawTypeEnum::Bayer;
							break;
						default:
							return nullptr;
					}
					break;
				case ExifTagsEnum::StripByteCounts:
					for (size_t i = 0; i < iterator->count(); i++)
					{
						metadata->dataSize += static_cast<int>(iterator->toUint32(i));
					}
					break;
				case ExifTagsEnum::CFAPattern2:
					metadata->cfaColorPattern.resize(iterator->count());
					for (size_t i = 0; i < iterator->count(); i++)
					{
						metadata->cfaColorPattern[i] = static_cast<Metadata::CFAPatternEnum>(iterator->toUint32(i));
					}
					break;
				case ExifTagsEnum::BlackLevel:
					metadata->blackLevels.resize(iterator->count());
					for (size_t i = 0; i < iterator->count(); i++)
					{
						metadata->blackLevels[i] = iterator->toUint32(i);
					}
					break;
				case ExifTagsEnum::WhiteLevel:
					metadata->whiteLevels.resize(iterator->count());
					for (size_t i = 0; i < iterator->count(); i++)
					{
						metadata->whiteLevels[i] = iterator->toUint32(i);
					}
					break;
				case ExifTagsEnum::ActiveArea:
					metadata->activeArea.resize(iterator->count());
					for (size_t i = 0; i < iterator->count(); i++)
					{
						metadata->activeArea[i] = iterator->toUint32(i);
					}
			}
		}
	}

	if (metadata->blackLevels.size() != metadata->getChannelsCount())
	{
		uint16_t blackLevel = 0;
		if (!metadata->blackLevels.isEmpty())
		{
			blackLevel = metadata->blackLevels[0];
		}

		metadata->blackLevels.resize(metadata->getChannelsCount());

		for (int i = 0; i < metadata->blackLevels.size(); i++)
		{
			metadata->blackLevels[i] = blackLevel;

		}
	}

	if (metadata->whiteLevels.size() != metadata->getChannelsCount())
	{
		uint16_t whiteLevel = 65535;
		if (!metadata->whiteLevels.isEmpty())
		{
			whiteLevel = metadata->whiteLevels[0];
		}

		metadata->whiteLevels.resize(metadata->getChannelsCount());

		for (int i = 0; i < metadata->whiteLevels.size(); i++)
		{
			metadata->whiteLevels[i] = whiteLevel;
		}
	}

	if (metadata->activeArea.isEmpty())
	{
		metadata->activeArea.resize(4);
		metadata->activeArea[0] = 0;
		metadata->activeArea[1] = 0;
		metadata->activeArea[2] = metadata->imageHeight;
		metadata->activeArea[3] = metadata->imageWidth;
	}

	if (metadata->isValid())
	{
		return metadata;
	}

	return nullptr;
}

int MetadataReader::findRawIFD(Exiv2::ExifData* exifData, int* photometricInterpretation)
{
	for (Exiv2::ExifData::const_iterator iterator = exifData->begin(); iterator != exifData->end(); ++iterator)
	{
		if (iterator->tag() == ExifTagsEnum::PhotometricInterpretation)
		{
			if (iterator->toUint32() == 34892 || iterator->toUint32() == 32803)
			{
				*photometricInterpretation = static_cast<int>(iterator->toUint32());
				return static_cast<int>(iterator->ifdId());
			}
		}
	}
	return -1;
}