#!/usr/bin/env python3

# This script automatically sets up Steam avatars, nicknames, and gathers the SteamID32 for bots.
# Change make_commands to False if you wish to get just the SteamID32, instead of Cathook's change playerstate command
# You do not have to "set up" a steam profile on each account for this to work, evidently.
# Simply copy your accounts.txt and bot-profile.jpg here and run: ./auto-profile.py
# Image format can be PNG, this script just expects the filename to be .jpg

# Make sure you install dependencies first:
# pip3 install -U steam[client]

import json
import time
import steam.client
from concurrent.futures import ThreadPoolExecutor, as_completed

f = open('accounts.txt', 'r')
data = f.read()
f.close()

data = data.replace('\r\n', '\n')
accounts = data.split('\n')
accounts = [account for account in accounts if account.strip()]  # we don't want empty strings.

profile = open('bot-profile.jpg', 'rb')
nickname = 'dallig'

# Disable features we don't need
enable_debugging = False
enable_extra_info = False
enable_avatarchange = False
enable_namechange = True
enable_nameclear = False  # Disabled to avoid session requirements
enable_set_up = False    # Disabled to avoid session requirements
enable_gatherid32 = True  # Enable gathering SteamID32
dump_response = False
make_commands = True
force_sleep = False

def debug(message):
    if enable_debugging:
        print(message)

def extra(message):
    if enable_extra_info:
        print(message)

def process_account(account_data):
    try:
        username, password = account_data.split(':')
        print(f'Processing account: {username}...')

        client = steam.client.SteamClient()
        eresult = client.login(username, password=password)
        status = 'OK' if eresult == 1 else 'FAIL'
        print(f'Login status for {username}: {status} ({eresult})')
        
        if status == 'FAIL':
            print(f'Failed to login {username}')
            return False, account_data, None

        print(f'Logged in as: {client.user.name}')
        print(f'Community profile: {client.steam_id.community_url}')
        
        if enable_namechange:
            try:
                client.change_status(persona_state=1, player_name=nickname)
                print(f'Changed Steam nickname to "{nickname}" for {username}')
            except Exception as e:
                print(f'Failed to change nickname for {username}: {str(e)}')

        if enable_gatherid32:
            try:
                steamid32 = client.steam_id.as_32
                with open('steamids', 'a') as f:
                    f.write(f'cat_pl_add_id {steamid32} CAT\n')
                    f.write(f'cat_pl_add_id {steamid32} CAT\n')
                print(f'Added SteamID32 {steamid32} to steamids file')
            except Exception as e:
                print(f'Failed to save SteamID32 for {username}: {str(e)}')

        # Save profile URL
        profile_url = client.steam_id.community_url
        with open('steam_profiles.txt', 'a') as f:
            f.write(f'{profile_url}\n')

        client.logout()
        return True, account_data, profile_url
    except Exception as e:
        print(f'Error processing account {username}: {str(e)}')
        return False, account_data, None

# Maximum number of concurrent operations
MAX_WORKERS = 10

# Lists to store working accounts and their data
working_accounts = []
working_profiles = []

# Process accounts concurrently
with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
    future_to_account = {executor.submit(process_account, account): account for account in accounts}
    
    for future in as_completed(future_to_account):
        account = future_to_account[future]
        try:
            success, account_data, profile_url = future.result()
            if success:
                working_accounts.append(account_data)
        except Exception as e:
            print(f'Account processing generated an exception: {str(e)}')

# Save working accounts to new accounts.txt
with open('accounts2.txt', 'w') as f:
    f.write('\n'.join(working_accounts))

print('Done processing all accounts.')

# Seek to the beginning of the profile image file; reuse the file
profile.seek(0)

print('Done.')
