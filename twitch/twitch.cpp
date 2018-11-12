#include "twitch.h"
#include <iostream>
#include <regex>
#include <fstream>
#include "../utf8/utf8.h"
#include "cpp_aux.h"

void event_connect( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {
	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}
	
	if ( irc_cmd_join( session, ctx->twitch->channel.c_str(), 0 ) ) {
		ctx->twitch->OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
		return;
	}

	ctx->twitch->status = TWITCH_CONNECTED;
	ctx->twitch->OnConnected();
}

void event_channel( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {

	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}

	std::string originalMessage = params[1];

	ctx->twitch->OnMessage( origin, originalMessage );

	static std::map<int, std::string> translitDictionary = {
		{ 1040, "A" }, // А
		{ 1072, "a" }, // а
		{ 1041, "B" }, // Б
		{ 1073, "b" }, // б
		{ 1042, "V" }, // В
		{ 1074, "v" }, // в
		{ 1043, "G" }, // Г
		{ 1075, "g" }, // г
		{ 1044, "D" }, // Д
		{ 1076, "d" }, // д
		{ 1045, "E" }, // Е
		{ 1077, "e" }, // е
		{ 1025, "Yo" }, // Ё
		{ 1105, "yo" }, // ё
		{ 1046, "Zh" }, // Ж
		{ 1078, "zh" }, // ж
		{ 1047, "Z" }, // З
		{ 1079, "z" }, // з
		{ 1048, "Y" }, // И
		{ 1080, "i" }, // и
		{ 1049, "J" }, // Й
		{ 1081, "j" }, // й
		{ 1050, "K" }, // К
		{ 1082, "k" }, // к
		{ 1051, "L" }, // Л
		{ 1083, "l" }, // л
		{ 1052, "M" }, // М
		{ 1084, "m" }, // м
		{ 1053, "N" }, // Н
		{ 1085, "n" }, // н
		{ 1054, "O" }, // О
		{ 1086, "o" }, // о
		{ 1055, "P" }, // П
		{ 1087, "p" }, // п
		{ 1056, "R" }, // Р
		{ 1088, "r" }, // р
		{ 1057, "S" }, // С
		{ 1089, "s" }, // с
		{ 1058, "T" }, // Т
		{ 1090, "t" }, // т
		{ 1059, "U" }, // У
		{ 1091, "u" }, // у
		{ 1060, "F" }, // Ф
		{ 1092, "f" }, // ф
		{ 1061, "Kh" }, // Х
		{ 1093, "kh" }, // х
		{ 1062, "Ts" }, // Ц
		{ 1094, "ts" }, // ц
		{ 1063, "Ch" }, // Ч
		{ 1095, "ch" }, // ч
		{ 1064, "Sh" }, // Ш
		{ 1096, "sh" }, // ш
		{ 1065, "Shh" }, // Щ
		{ 1097, "shh" }, // щ
		{ 1067, "Y" }, // Ы
		{ 1099, "y" }, // ы
		{ 1069, "E" }, // Э
		{ 1101, "e" }, // э
		{ 1070, "Yu" }, // Ю
		{ 1102, "yu" }, // ю
		{ 1071, "Ya" }, // Я
		{ 1103, "ya" }, // я
		{ 1068, "" }, // Ь
		{ 1100, "" }, // ь
		{ 1066, "" }, // Ъ
		{ 1098, "" }, // ъ
	};

	std::string message = "";
	message.reserve( originalMessage.size() );

	auto it = originalMessage.begin();
	while ( it != originalMessage.end() ) {
		auto cp = utf8::peek_next( it, originalMessage.end() );
		if ( translitDictionary.find( cp ) != translitDictionary.end() ) {
			message += translitDictionary[cp];
		} else {
			message += *it;
		}
		utf8::advance( it, 1, originalMessage.end() );
	}

	aux::str::replace( &message, "[^ -~]", "" );
	aux::str::replace( &message, "\\s+", " " );
	aux::str::replace( &message, "^[#!0]*([1-9]{1})", "" );

	aux::str::trim( &message );

	if ( message.size() > 0 && message.size() < 64 ) {
		auto &killfeedMessages = ctx->twitch->killfeedMessages;
		killfeedMessages.push_back( message );
		if ( killfeedMessages.size() > 512 ) {
			killfeedMessages.pop_front();
		}
	}
}

void event_numeric( irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count ) {
	if ( event > 400 ) {
		std::string fulltext;
		for ( unsigned int i = 0; i < count; i++ ) {
			if ( i > 0 )
				fulltext += " ";

			fulltext += params[i];
		}

		TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
		if ( !ctx ) {
			return;
		}
		ctx->twitch->OnError( event, fulltext );
	}
}

Twitch::Twitch() {
}

Twitch::~Twitch() {
	if ( session ) {
		TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
		if ( ctx ) {
			delete ctx;
		}

		Disconnect();
	}
}

std::thread Twitch::Connect( const std::string &user, const std::string &password ) {
	return std::thread( [this, user, password] {
		this->user = user;
		this->channel = "#" + user;

		irc_callbacks_t	callbacks;
		memset( &callbacks, 0, sizeof( callbacks ) );
		callbacks.event_connect = event_connect;
		callbacks.event_channel = event_channel;
		callbacks.event_numeric = event_numeric;

		session = irc_create_session( &callbacks );

		if ( !session ) {
			OnError( -1, "Failed to create IRC session" );
			return;
		}

		TwitchContext *ctx = new TwitchContext();
		ctx->twitch = this;
		irc_set_ctx( session, ctx );

		irc_option_set( session, LIBIRC_OPTION_STRIPNICKS );

		status = TWITCH_CONNECTING;

		if ( irc_connect( session, "irc.chat.twitch.tv", 6667, password.c_str(), user.c_str(), user.c_str(), user.c_str() ) ) {
			OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
			status = TWITCH_DISCONNECTED;
			return;
		}

		if ( irc_run( session ) ) {
			OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
			status = TWITCH_DISCONNECTED;
			return;
		}
	} );
}

void Twitch::Disconnect() {
	if ( session ) {
		irc_disconnect( session );
		
		status = TWITCH_DISCONNECTED;
		OnDisconnected();
	}
}

void Twitch::SendChatMessage( const std::string &message ) {
	if ( !session ) {
		return;
	}

	if ( irc_cmd_msg( session, channel.c_str(), message.c_str() ) ) {
		OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
	}
}

