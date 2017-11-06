#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"

#include "aim_entity.h"

bool aimingAtEntity = false;
std::string entityClassName = "";
std::string entityTarget = "";
std::string entityTargetName = "";
int entityModelIndex = 0;
Vector entityPos = Vector( 0, 0, 0 );
Vector entityAngle = Vector( 0, 0, 0 );
Vector entityVelocity = Vector( 0, 0, 0 );
Vector entityMins = Vector( 0, 0, 0 );
Vector entityMaxs = Vector( 0, 0, 0 );

float entityHealth = 0.0f;
float entityHealthMax = 0.0f;
int entityFlags = 0;
int entityDeadFlags = 0;
int entitySpawnFlags = 0;

void AimEntity_Init() {
	gEngfuncs.pfnHookUserMsg( "OnAimNew", AimEntity_OnAimNew );
	gEngfuncs.pfnHookUserMsg( "OnAimUpd", AimEntity_OnAimUpd );
	gEngfuncs.pfnHookUserMsg( "OnAimClear", AimEntity_OnAimClear );
}

int AimEntity_OnAimNew( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	entityClassName = READ_STRING();
	entityTarget = READ_STRING();
	entityTargetName = READ_STRING();
	entityModelIndex = READ_SHORT();
	entityMins = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	entityMaxs = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	entitySpawnFlags = READ_BYTE();

	return 1;
}

int AimEntity_OnAimUpd( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	entityPos = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	entityAngle = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	entityVelocity = Vector( READ_COORD(), READ_COORD(), READ_COORD() );
	entityHealth = READ_FLOAT();
	entityHealthMax = READ_FLOAT();
	entityFlags = READ_BYTE();
	entityDeadFlags = READ_BYTE();

	return 1;
}

int AimEntity_OnAimClear( const char *pszName, int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	return 1;
}

// 1st column is left aligned though
void AimEntity_DrawFourColumnRowRightAligned( const char *col1, const char *col2, const char *col3, const char *col4 ) {
	const int PADDING = 16;

	ImGui::Text( col1 ); ImGui::NextColumn();	

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col2 ).x - PADDING );
	ImGui::Text( col2 ); ImGui::NextColumn();

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col3 ).x - PADDING );
	ImGui::Text( col3 ); ImGui::NextColumn();

	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize( col4 ).x - PADDING );
	ImGui::Text( col4 ); ImGui::NextColumn();
}

void AimEntity_DrawFourColumnRowRightAligned( const char *name, const Vector &vec ) {
	char col2[256]; sprintf( col2, "%.2f", vec.x );
	char col3[256]; sprintf( col3, "%.2f", vec.y );
	char col4[256]; sprintf( col4, "%.2f", vec.z );

	AimEntity_DrawFourColumnRowRightAligned( name, col2, col3, col4 );
}

void AimEntity_Draw() {
	ImGui::SetNextWindowPos( ImVec2( 60, 60 ), ImGuiSetCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 350, 200 ), ImGuiSetCond_FirstUseEver );

	ImGui::Begin( "Aim entity" );

	ImGui::Columns( 2, "aim_entity_columns" );
	ImGui::Text( "Class name" ); ImGui::NextColumn();
	ImGui::Text( entityClassName.c_str() ); ImGui::NextColumn();

	ImGui::Text( "Model index" ); ImGui::NextColumn();
	ImGui::Text( "%d", entityModelIndex ); ImGui::NextColumn();

	ImGui::Text( "Target" ); ImGui::NextColumn();
	ImGui::Text( entityTarget.c_str() ); ImGui::NextColumn();

	ImGui::Text( "Target name" ); ImGui::NextColumn();
	ImGui::Text( entityTargetName.c_str() ); ImGui::NextColumn();

	ImGui::Text( "Health" ); ImGui::NextColumn();
	ImGui::Text( "%.0f / %.0f", entityHealth, entityHealthMax ); ImGui::NextColumn();

	ImGui::Text( "Flags" ); ImGui::NextColumn();
	ImGui::Text( "%d", entityFlags ); ImGui::NextColumn();

	ImGui::Text( "Dead flags" ); ImGui::NextColumn();
	ImGui::Text( "%d", entityDeadFlags ); ImGui::NextColumn();

	ImGui::Text( "Spawn flags" ); ImGui::NextColumn();
	ImGui::Text( "%d", entitySpawnFlags ); ImGui::NextColumn();

	ImGui::Columns( 4, "aim_entity_vec_columns" );
	AimEntity_DrawFourColumnRowRightAligned( "", "x", "y", "z" );

	AimEntity_DrawFourColumnRowRightAligned( "Position", entityPos );
	AimEntity_DrawFourColumnRowRightAligned( "Angle", entityAngle );
	AimEntity_DrawFourColumnRowRightAligned( "Velocity", entityVelocity );
	AimEntity_DrawFourColumnRowRightAligned( "Mins", entityMins );
	AimEntity_DrawFourColumnRowRightAligned( "Maxs", entityMaxs );

	Vector minMaxs = Vector( ( entityMins.x + entityMaxs.x ) / 2.0f, ( entityMins.y + entityMaxs.y ) / 2.0f, ( entityMins.z + entityMaxs.z ) / 2.0f );
	AimEntity_DrawFourColumnRowRightAligned( "Min/Maxs", minMaxs );
	AimEntity_DrawFourColumnRowRightAligned( "Min/Maxs + Pos.", entityPos + minMaxs );

	ImGui::End();
}