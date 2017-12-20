#ifndef END_MARKER_H
#define END_MARKER_H


class CEndMarker : public CBaseToggle {
public:
	void			Spawn( void );
	void			Precache( void );
	void EXPORT		OnTouch( CBaseEntity *pOther );
	void EXPORT		OnUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );

	BOOL			active;

	static TYPEDESCRIPTION m_SaveData[];
};

#endif // END_MARKER_H
