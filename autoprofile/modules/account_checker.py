import sys
import threading
import time
from pathlib import Path
from dataclasses import dataclass
from typing import Callable

from autoprofile.core import paths
from autoprofile.modules.profile_update import login_with_timeout

try:
    import steam.client
    from steam.enums import EResult
except ImportError as exc:
    steam = None
    EResult = None
    steam_import_error = exc
else:
    steam_import_error = None

@dataclass
class AccountCheckerResult:
    total: int = 0
    success: int = 0
    failed: int = 0
    duration_seconds: float = 0.0
    error: str = None

def run_account_checker(settings, log_callback: Callable[[str], None], stop_event: threading.Event) -> AccountCheckerResult:
    started_at = time.time()
    
    if steam_import_error is not None:
        return AccountCheckerResult(error=f'Missing Python dependency: steam[client] ({steam_import_error})')

    def safe_print(msg):
        if log_callback:
            log_callback(msg)

    def wait_or_stop(seconds):
        return stop_event.wait(seconds)

    accounts_file = paths.data_path('accounts.txt')
    if not accounts_file.exists():
        safe_print('accounts.txt not found. Please create it or use the Web UI to add accounts.')
        return AccountCheckerResult(error='accounts.txt not found')

    lines = accounts_file.read_text(encoding='utf-8').splitlines()
    accounts = [line.strip() for line in lines if line.strip()]
    if not accounts:
        safe_print('accounts.txt is empty.')
        return AccountCheckerResult(error='No accounts found')

    safe_print(f'Starting Account Checker for {len(accounts)} accounts.')

    max_login_retries = getattr(settings, 'max_login_retries', 3)
    login_timeout_seconds = getattr(settings, 'login_timeout_seconds', 45)

    good_accounts = []
    bad_accounts = []

    for index, account_line in enumerate(accounts):
        if stop_event.is_set():
            safe_print('Account Checker stopped by user.')
            break

        try:
            username, password = account_line.split(':', 1)
        except ValueError:
            safe_print(f'[#{index + 1}] Invalid account line format. Skipping.')
            bad_accounts.append(account_line)
            continue
        safe_print(f'[#{index + 1}] Checking {username}...')

        login_successful = False
        client = None

        for retry in range(max_login_retries):
            if stop_event.is_set():
                break

            client = steam.client.SteamClient()
            try:
                eresult = login_with_timeout(client, username, password, login_timeout_seconds)
            except Exception as exc:
                safe_print(f'[#{index + 1}] Login encountered an error: {exc}')
                eresult = EResult.Fail

            try:
                eresult_enum = EResult(eresult)
            except (TypeError, ValueError):
                eresult_enum = EResult.Fail

            status = 'OK' if eresult_enum == EResult.OK else 'FAIL'
            safe_print(f'[#{index + 1}] Login status: {status} - {eresult_enum.name}')

            if eresult_enum == EResult.OK:
                login_successful = True
                break

            if eresult_enum == EResult.AccountDisabled:
                safe_print(f'[#{index + 1}] Account is Disabled (E43). Will not retry.')
                break
                
            if eresult_enum != EResult.InvalidPassword:

                pass

            if retry < max_login_retries - 1:
                safe_print(f'[#{index + 1}] Retrying login... (attempt {retry + 2}/{max_login_retries})')
                if wait_or_stop(2):
                    break

            try:
                client.disconnect()
            except Exception:
                pass

        try:
            if client:
                client.disconnect()
        except Exception:
            pass

        if login_successful:
            good_accounts.append(account_line)
            safe_print(f'[#{index + 1}] ✓ Account works.')
        else:
            bad_accounts.append(account_line)
            safe_print(f'[#{index + 1}] ⚠ Account failed check.')

    duration = time.time() - started_at
    
    safe_print('\n=== Account Checker Summary ===')
    safe_print(f'Total accounts checked: {len(accounts)}')
    safe_print(f'Successful accounts: {len(good_accounts)}')
    safe_print(f'Failed accounts: {len(bad_accounts)}')
    
    if good_accounts:
        good_path = paths.data_path('goodaccounts.txt')
        good_path.write_text('\n'.join(good_accounts) + '\n', encoding='utf-8')
        safe_print(f'Good accounts saved to: goodaccounts.txt')
        
    if bad_accounts:
        bad_path = paths.data_path('badaccounts.txt')
        bad_path.write_text('\n'.join(bad_accounts) + '\n', encoding='utf-8')
        safe_print(f'Bad accounts saved to: badaccounts.txt')
        
    safe_print(f'Job finished in {duration:.1f}s')

    return AccountCheckerResult(
        total=len(accounts),
        success=len(good_accounts),
        failed=len(bad_accounts),
        duration_seconds=duration
    )
