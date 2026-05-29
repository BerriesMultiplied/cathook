#ifndef BHOP_HPP
#define BHOP_HPP

struct user_cmd;

void bhop(user_cmd* user_cmd);
bool moonwalk_create_move(user_cmd* user_cmd);
bool moonwalk_applied_to_command(int command_number);

#endif // BHOP_HPP
