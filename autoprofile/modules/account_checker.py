import threading
import time
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
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
class account_checker_result:
    total: int = 0
    success: int = 0
    failed: int = 0
    duration_seconds: float = 0.0
    error: str = None


@dataclass
class account_result:
    index: int
    account_line: str
    success: bool = False
    skipped: bool = False


def login_should_retry(eresult_enum) -> bool:
    name = getattr(eresult_enum, 'name', str(eresult_enum))
    if any(token in name for token in (
        'Invalid',
        'Disabled',
        'Denied',
        'TwoFactor',
        'NoMail',
        'LimitExceeded',
        'AccountAssociated'
    )):
        return False
    return name in {
        'Fail',
        'Timeout',
        'NoConnection',
        'ServiceUnavailable',
        'Busy',
        'TryAnotherCM',
        'IOFailure',
        'RemoteCallFailed'
    }


def check_account(index, account_line, max_login_retries, login_timeout_seconds, stop_event, safe_print):
    if stop_event.is_set():
        return account_result(index=index, account_line=account_line, skipped=True)

    try:
        username, password = account_line.split(':', 1)
    except ValueError:
        safe_print(f'[#{index + 1}] Invalid account line format. Skipping.')
        return account_result(index=index, account_line=account_line)

    safe_print(f'[#{index + 1}] Checking {username}...')

    for retry in range(max_login_retries):
        if stop_event.is_set():
            return account_result(index=index, account_line=account_line, skipped=True)

        client = steam.client.SteamClient()
        try:
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
                safe_print(f'[#{index + 1}] Account works.')
                return account_result(index=index, account_line=account_line, success=True)

            if not login_should_retry(eresult_enum):
                safe_print(f'[#{index + 1}] Permanent login failure. Will not retry.')
                break

            if retry < max_login_retries - 1:
                safe_print(f'[#{index + 1}] Retrying login... (attempt {retry + 2}/{max_login_retries})')
                if stop_event.wait(1):
                    return account_result(index=index, account_line=account_line, skipped=True)
        finally:
            try:
                client.disconnect()
            except Exception:
                pass

    safe_print(f'[#{index + 1}] Account failed check.')
    return account_result(index=index, account_line=account_line)


def run_account_checker(settings, log_callback: Callable[[str], None], stop_event: threading.Event) -> account_checker_result:
    started_at = time.time()

    if steam_import_error is not None:
        return account_checker_result(error=f'Missing Python dependency: steam[client] ({steam_import_error})')

    def safe_print(msg):
        if log_callback:
            log_callback(msg)

    accounts_file = paths.data_path('accounts.txt')
    if not accounts_file.exists():
        safe_print('accounts.txt not found. Please create it or use the Web UI to add accounts.')
        return account_checker_result(error='accounts.txt not found')

    lines = accounts_file.read_text(encoding='utf-8').splitlines()
    accounts = [line.strip() for line in lines if line.strip()]
    if not accounts:
        safe_print('accounts.txt is empty.')
        return account_checker_result(error='No accounts found')

    max_parallel_accounts = max(1, min(64, int(getattr(settings, 'max_parallel_accounts', 8))))
    max_login_retries = max(1, min(10, int(getattr(settings, 'max_login_retries', 3))))
    login_timeout_seconds = max(5, min(20, int(getattr(settings, 'login_timeout_seconds', 20))))
    max_workers = min(max_parallel_accounts, len(accounts))

    safe_print(f'Starting Account Checker for {len(accounts)} accounts with {max_workers} workers.')
    safe_print(f'Checker timeout is {login_timeout_seconds}s per login attempt.')

    results = []

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        future_map = {
            executor.submit(
                check_account,
                index,
                account_line,
                max_login_retries,
                login_timeout_seconds,
                stop_event,
                safe_print
            ): index
            for index, account_line in enumerate(accounts)
        }
        pending = set(future_map)
        while pending:
            done, pending = wait(pending, timeout=0.5, return_when=FIRST_COMPLETED)
            for future in done:
                index = future_map[future]
                try:
                    results.append(future.result())
                except Exception as exc:
                    safe_print(f'[#{index + 1}] Unexpected checker error: {exc}')
                    results.append(account_result(index=index, account_line=accounts[index]))
            if stop_event.is_set():
                safe_print('Account Checker stopped by user.')
                for future in pending:
                    future.cancel()
                break

    ordered_results = sorted(results, key=lambda item: item.index)
    good_accounts = [item.account_line for item in ordered_results if item.success]
    bad_accounts = [item.account_line for item in ordered_results if not item.success and not item.skipped]
    checked_count = len(ordered_results) if stop_event.is_set() else len(accounts)
    duration = time.time() - started_at

    safe_print('\n=== Account Checker Summary ===')
    safe_print(f'Total accounts checked: {checked_count}')
    safe_print(f'Successful accounts: {len(good_accounts)}')
    safe_print(f'Failed accounts: {len(bad_accounts)}')

    good_path = paths.data_path('goodaccounts.txt')
    good_path.write_text('\n'.join(good_accounts) + ('\n' if good_accounts else ''), encoding='utf-8')
    safe_print('Good accounts saved to: goodaccounts.txt')

    bad_path = paths.data_path('badaccounts.txt')
    bad_path.write_text('\n'.join(bad_accounts) + ('\n' if bad_accounts else ''), encoding='utf-8')
    safe_print('Bad accounts saved to: badaccounts.txt')

    safe_print(f'Job finished in {duration:.1f}s')

    return account_checker_result(
        total=checked_count,
        success=len(good_accounts),
        failed=len(bad_accounts),
        duration_seconds=duration
    )
