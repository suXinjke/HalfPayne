﻿#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"

#include "player_info_window.h"

Vector playerOrigin = Vector( 0, 0, 0 );
Vector playerViewOffset = Vector( 0, 0, 0 );
Vector playerViewAngles = Vector( 0, 0, 0 );
Vector playerViewHitPos = Vector( 0, 0, 0 );
Vector playerVelocity = Vector( 0, 0, 0 );

void PlayerInfoWindow_Init() {
	gEngfuncs.pfnHookUserMsg( "OnPlyUpd", PlayerInfoWindow_OnPlyUpd );
}

// DRY ISSUE WITH aim_entity.cpp
void PlayerInfoWindow_DrawFourColumnRowRightAligned( const char *col1, const char *col2, const char *col3, const char *col4 ) {
	const int PADDING = 16;

	ImGui::Text( col1 ); ImGui::NextColumn();	

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col2 ).x - PADDING );
	ImGui::Text( col2 ); ImGui::NextColumn();

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col3 ).x - PADDING );
	ImGui::Text( col3 ); ImGui::NextColumn();

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col4 ).x - PADDING );
	ImGui::Text( col4 ); ImGui::NextColumn();
}

void PlayerInfoWindow_DrawFourColumnRowRightAligned( const char *name, const Vector &vec ) {
	char col2[256]; sprintf( col2, "%.2f", vec.x );
	char col3[256]; sprintf( col3, "%.2f", vec.y );
	char col4[256]; sprintf( col4, "%.2f", vec.z );

	PlayerInfoWindow_DrawFourColumnRowRightAligned( name, col2, col3, col4 );
}

int PlayerInfoWindow_OnPlyUpd( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	playerViewHitPos = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	playerOrigin = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	playerViewOffset = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	playerViewAngles = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	playerVelocity = Vector( READ_COORD(), READ_COORD(), READ_COORD() );

	return 1;
}

void PlayerInfoWindow_Draw() {
	ImGui::SetNextWindowPos( ImVec2( 60, 60 ), ImGuiSetCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 350, 70 ), ImGuiSetCond_FirstUseEver );

	ImGui::Begin( "Player info" );
	ImGui::Columns( 4, "player_info_vec_columns" );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "", "x", "y", "z" );

	PlayerInfoWindow_DrawFourColumnRowRightAligned( "Position", playerOrigin );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "Position (view)", playerOrigin + playerViewOffset );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "Position (aim)", playerViewHitPos );

	char distance[256];
	sprintf( distance, "%.2f", ( playerViewHitPos - ( playerOrigin + playerViewOffset ) ).Length() );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "Distance", distance, "", "" );

	PlayerInfoWindow_DrawFourColumnRowRightAligned( "View angle", playerViewAngles );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "Velocity", playerVelocity );

	char velocity[256];
	sprintf( velocity, "%.2f", playerVelocity.Length() );
	PlayerInfoWindow_DrawFourColumnRowRightAligned( "", velocity, "", "" );

	ImGui::End();
}