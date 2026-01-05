#ifndef REPLIES_HPP
#define REPLIES_HPP

/**
 * @brief IRC numeric reply codes
 * 
 * Defines all IRC numeric reply codes used by the server for successful
 * responses (RPL_) and error messages (ERR_) according to RFC 1459.
 */

// Successful replies (001-099)
#define RPL_WELCOME				001
#define RPL_YOURHOST			002
#define RPL_CREATED				003
#define RPL_MYINFO				004
#define RPL_UMODEIS				221
#define RPL_ENDOFWHO			315
#define RPL_CHANNELMODEIS		324
#define RPL_NOTOPIC				331
#define RPL_TOPIC				332
#define RPL_INVITING			341
#define RPL_WHOREPLY			352
#define RPL_NAMREPLY			353
#define RPL_ENDOFNAMES			366

// Error replies (400-599)
#define ERR_NOSUCHNICK			401
#define ERR_NOSUCHCHANNEL		403
#define ERR_CANNOTSENDTOCHAN	404
#define ERR_NORECIPIENT			411
#define ERR_NOTEXTTOSEND		412
#define ERR_UNKNOWNCOMMAND		421
#define ERR_NONICKNAMEGIVEN		431
#define ERR_ERRONEOUSNICKNAME	432
#define ERR_NICKNAMEINUSE		433
#define ERR_USERNOTINCHANNEL	441
#define ERR_NOTONCHANNEL		442
#define ERR_USERONCHANNEL		443
#define ERR_NOTREGISTERED		451
#define ERR_NEEDMOREPARAMS		461
#define ERR_ALREADYREGISTERED	462
#define ERR_PASSWDMISMATCH		464
#define ERR_CHANNELISFULL		471
#define ERR_UNKNOWNMODE			472
#define ERR_INVITEONLYCHAN		473
#define ERR_BADCHANNELKEY		475
#define ERR_CHANOPRIVSNEEDED	482
#define ERR_USERSDONTMATCH		501
#define ERR_UMODEUNKNOWNFLAG	502


#endif
