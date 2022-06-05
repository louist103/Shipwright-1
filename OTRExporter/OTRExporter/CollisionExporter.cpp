#include "CollisionExporter.h"
#include <Resource.h>
#include "Globals.h"

void OTRExporter_Collision::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZCollisionHeader* col = (ZCollisionHeader*)res;

	WriteHeader(res, outPath, writer, Ship::ResourceType::CollisionHeader);
	
	writer->Write(col->absMinX);
	writer->Write(col->absMinY);
	writer->Write(col->absMinZ);

	writer->Write(col->absMaxX);
	writer->Write(col->absMaxY);
	writer->Write(col->absMaxZ);

	writer->Write((uint32_t)col->vertices.size());
	
	for (uint16_t i = 0; i < col->vertices.size(); i++)
	{
		writer->Write(col->vertices[i].scalars[0].scalarData.s16);
		writer->Write(col->vertices[i].scalars[1].scalarData.s16);
		writer->Write(col->vertices[i].scalars[2].scalarData.s16);
	}

	writer->Write((uint32_t)col->polygons.size());

	for (uint16_t i = 0; i < col->polygons.size(); i++)
	{
		writer->Write(col->polygons[i].type);
		writer->Write(col->polygons[i].vtxA);
		writer->Write(col->polygons[i].vtxB);
		writer->Write(col->polygons[i].vtxC);
		writer->Write(col->polygons[i].normX);
		writer->Write(col->polygons[i].normY);
		writer->Write(col->polygons[i].normZ);
		writer->Write(col->polygons[i].dist);
	}

	writer->Write((uint32_t)col->polygonTypes.size());

	for (uint16_t i = 0; i < col->polygonTypes.size(); i++) {
		writer->Write(col->polygonTypes[i].data[0]);
		writer->Write(col->polygonTypes[i].data[1]);
	}

	for (const auto& file : Globals::Instance->files) {
		for (const auto& resource : file->resources) {
			if (resource->GetRawDataIndex() == col->camDataAddress) {
				ZCamData* camData = (ZCamData*)resource;

				writer->Write((uint32_t)camData->camPosData.size());

				for (auto entry : camData)
				{
					auto camPosDecl = col->parent->GetDeclarationRanged(Seg2Filespace(entry->cameraPosDataSeg, col->parent->baseAddress));

					int idx = 0;

					if (camPosDecl != nullptr)
						idx = ((entry->cameraPosDataSeg & 0x00FFFFFF) - camPosDecl->address) / 6;

					writer->Write(entry->cameraSType);
					writer->Write(entry->numData);
					writer->Write((uint32_t)idx);
				}

				writer->Write((uint32_t)camData->camPosData.size());

				for (auto entry : camData->camPosData)
				{
					writer->Write(entry.x);
					writer->Write(entry.y);
					writer->Write(entry.z);
				}

			}
		}
	}


	writer->Write((uint32_t)col->waterBoxes.size());

	for (auto waterBox : col->waterBoxes)
	{
		writer->Write(waterBox.xMin);
		writer->Write(waterBox.ySurface);
		writer->Write(waterBox.zMin);
		writer->Write(waterBox.xLength);
		writer->Write(waterBox.zLength);
		writer->Write(waterBox.properties);
	}
}
