#ifndef KEROTAN_H
#define KEROTAN_H


class CKerotan : public CBaseToggle {
public:
	void			Spawn( void );
	void			Precache( void );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	virtual int		TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;

	BOOL			hasBeenFound;
	int				hitsReceived;
	int				soundsLeft;
	float			nextSound;
	float			rotationLeft;
	int				rotationDirection;
	float			rollAmplitude;
	int				rollDirection;

	float			nextCallout;

	void EXPORT		OnThink ();

	static TYPEDESCRIPTION m_SaveData[];
};

#endif // KEROTAN_H
