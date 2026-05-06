/*
 * cathookipc.hpp
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <ctime>

struct server_data_s
{
    unsigned int magic_number;
};

struct user_data_s
{
    char name[32];
    unsigned int friendid;
    bool textmode;

    bool connected;

    time_t heartbeat;

    time_t ts_injected;
    time_t ts_connected;
    time_t ts_disconnected;

    struct accumulated_t
    {
        int kills;        // TODO
        int deaths;        // TODO
        int score;

        int shots;
        int hits;
        int headshots;
    } accumulated;

    struct
    {
        bool good;

        int kills;        // TODO
        int deaths;        // TODO
        int score;

        int shots;        // TODO
        int hits;        // TODO
        int headshots;

        int team;
        int role; // class
        char life_state;
        int health;
        int health_max;

        float x;
        float y;
        float z;

        int player_count;
        int bot_count;

        char server[24];
        char mapname[32];
    } ingame;
};

namespace ipc_commands
{
    constexpr unsigned execute_client_cmd = 1;
    constexpr unsigned set_follow_steamid = 2;
    constexpr unsigned execute_client_cmd_long = 3;
    constexpr unsigned move_to_vector = 4;
    constexpr unsigned stop_moving = 5;
    constexpr unsigned start_moving = 6;
}