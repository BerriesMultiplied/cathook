import time
from autoprofile.modules.profile_update import profile_update_settings, run_profile_update, stop_event
settings = profile_update_settings()
settings.enable_namechange = False
settings.enable_avatarchange = False
settings.enable_descriptionchange = False
settings.enable_customurlchange = False
settings.enable_themechange = False
settings.enable_profile_verification = False
settings.enable_nameclear = True
settings.max_parallel_accounts = 1
run_profile_update(settings, print, stop_event)
