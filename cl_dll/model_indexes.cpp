#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"
#include <vector>

#include "model_indexes.h"

std::vector<ModelIndexMessage> modelIndexes;

void ModelIndexes_Init() {
	gEngfuncs.pfnHookUserMsg( "OnModelIdx", ModelIndexes_OnModelIdx );
}

void ModelIndexes_AddModelIndex( const ModelIndexMessage &modelIndex ) {
	modelIndexes.push_back( modelIndex );
}

int ModelIndexes_OnModelIdx( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	std::string map = READ_STRING();
	int index = READ_LONG();
	std::string target_name = READ_STRING();
	std::string class_name = READ_STRING();

	ModelIndexes_AddModelIndex( { map, index, target_name, class_name } );

	return 1;
}

void ModelIndexes_Draw() {
	ImGui::SetNextWindowPos( ImVec2( 60, 60 ), ImGuiSetCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 350, 200 ), ImGuiSetCond_FirstUseEver );

	ImGui::Begin( "Hooked model indexes" );

	static ImGuiTextFilter filter;
	ImGui::Text( "Class name filter" ); ImGui::SameLine();
	filter.Draw( "", -100.0f ); ImGui::SameLine();

	// REFRESH BUTTON
	if ( ImGui::Button( "Clear", ImVec2( -1, 0 ) ) ) {
		modelIndexes.clear();
	}

	ImGui::Columns( 4, "model_index_columns_headers" );
	ImGui::Separator();

	ImGui::Columns( 4, "model_index_columns" );
	ImGui::Text( "Map" ); ImGui::NextColumn();
	ImGui::Text( "Model index" ); ImGui::NextColumn();
	ImGui::Text( "Target name" ); ImGui::NextColumn();
	ImGui::Text( "Class name" ); ImGui::NextColumn();
	ImGui::Separator();

	for ( auto i = modelIndexes.rbegin(); i != modelIndexes.rend(); i++ ) {
		const auto &modelIndex = *i;

		if ( !filter.PassFilter( modelIndex.class_name.c_str() ) ) {
			continue;
		}

		ImGui::Text( modelIndex.map.c_str() ); ImGui::NextColumn();
		if ( modelIndex.index != 0 ) {
			ImGui::Text( "%d", modelIndex.index ); ImGui::NextColumn();
		} else {
			ImGui::Text( "-" ); ImGui::NextColumn();
		}
		ImGui::Text( modelIndex.target_name.size() > 0 ? modelIndex.target_name.c_str() : "-" ); ImGui::NextColumn();
		ImGui::Text( modelIndex.class_name.size() > 0 ? modelIndex.class_name.c_str() : "-" ); ImGui::NextColumn();
	}
	
	ImGui::End();
}