import threading
import time
from pathlib import Path

from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical, VerticalScroll, Grid
from textual.widgets import (
    Header, Footer, TabbedContent, TabPane, 
    Input, TextArea, Checkbox, Select, Button, RichLog, Label
)

from autoprofile.core import paths
from autoprofile.modules.profile_update import default_settings, run_profile_update, settings_from_dict
from autoprofile.modules.account_checker import run_account_checker


class AutoprofileTUI(App):
    TITLE = "auto_profile"

    BINDINGS = [
        ("ctrl+s", "save", "Save"),
        ("ctrl+q", "quit", "Quit"),
    ]

    CSS = """
    Screen {
        background: $surface;
    }
    #main_grid {
        grid-size: 2;
        grid-columns: 1fr 1fr;
        height: 60;
    }
    .form-col {
        padding: 1;
        height: 100%;
    }
    .input-box {
        height: 10;
        margin-bottom: 1;
    }
    .number-input {
        width: 100%;
    }
    .perf-group {
        width: 1fr;
        height: auto;
        margin-right: 1;
    }
    .actions {
        height: auto;
        margin-bottom: 1;
        margin-top: 1;
        align: center middle;
    }
    .actions Button {
        margin-right: 1;
    }
    .section-title {
        text-style: bold;
        color: $accent;
        margin-top: 1;
        margin-bottom: 1;
    }
    .switches-grid {
        grid-size: 2;
        height: auto;
    }
    RichLog {
        height: 15;
        border: solid $primary;
        margin-top: 1;
    }
    """

    def __init__(self):
        super().__init__()
        self.active_thread = None
        self.stop_event = None
        self.settings = default_settings().__dict__

    def compose(self) -> ComposeResult:
        yield Header()
        
        with TabbedContent():
            with TabPane("Main", id="main_tab"):
                with VerticalScroll():
                    with Horizontal(classes="actions"):
                        yield Button("Start", id="btn_start", variant="success")
                        yield Button("Stop", id="btn_stop", variant="error", disabled=True)
                        yield Button("Save", id="btn_save", variant="primary")
                    
                    with Grid(id="main_grid"):
                        with Vertical(classes="form-col"):
                            yield Label("Accounts")
                            yield TextArea(id="input_accounts", classes="input-box")
                            yield Label("Rollids")
                            yield TextArea(id="input_rollids", classes="input-box")
                            yield Label("Proxies")
                            yield TextArea(id="input_proxies", classes="input-box")
                            
                        with Vertical(classes="form-col"):
                            yield Label("Profile Values", classes="section-title")
                            
                            yield Label("Nickname")
                            yield Input(id="default_nickname")
                            
                            yield Label("Description")
                            yield Input(id="default_profile_summary")
                            
                            yield Label("Custom URL")
                            yield Input(id="default_custom_url")
                            
                            yield Label("Theme")
                            yield Select(
                                [("Default", ""), ("Summer", "Summer"), ("Midnight", "Midnight"), ("Steel", "Steel"), ("Cosmic", "Cosmic"), ("DarkMode", "DarkMode")],
                                id="default_profile_theme"
                            )
                            
                            yield Label("Avatar Path")
                            yield Input(id="profile_image_path")
                            
                            yield Label("Switches", classes="section-title")
                            with Grid(classes="switches-grid"):
                                yield Checkbox("Change name", id="enable_namechange")
                                yield Checkbox("Change avatar", id="enable_avatarchange")
                                yield Checkbox("Change description", id="enable_descriptionchange")
                                yield Checkbox("Change custom URL", id="enable_customurlchange")
                                yield Checkbox("Change theme", id="enable_themechange")
                                yield Checkbox("Verify profile", id="enable_profile_verification")
                                yield Checkbox("Clear aliases", id="enable_nameclear")
                                yield Checkbox("Write SteamID32", id="enable_gatherid32")
                                yield Checkbox("Random names", id="random_name")
                                yield Checkbox("Insert chars", id="insert_random_chars_enabled")
                                yield Checkbox("Scrape rollids", id="use_rollids")
                            
                            yield Label("Performance", classes="section-title")
                            with Horizontal():
                                with Vertical(classes="perf-group"):
                                    yield Label("Parallel")
                                    yield Input(id="max_parallel_accounts", classes="number-input")
                                with Vertical(classes="perf-group"):
                                    yield Label("Retries")
                                    yield Input(id="max_login_retries", classes="number-input")
                                with Vertical(classes="perf-group"):
                                    yield Label("Login Time")
                                    yield Input(id="login_timeout_seconds", classes="number-input")
                                with Vertical(classes="perf-group"):
                                    yield Label("Request Time")
                                    yield Input(id="request_timeout_seconds", classes="number-input")

                    yield Label("Live Logs:")
                    yield RichLog(id="main_log", wrap=True, highlight=True, markup=True)

            with TabPane("Checker", id="checker_tab"):
                with VerticalScroll():
                    with Horizontal(classes="actions"):
                        yield Button("Start Check", id="btn_start_check", variant="success")
                        yield Button("Stop Check", id="btn_stop_check", variant="error", disabled=True)
                    
                    yield Label("Checker Logs:")
                    yield RichLog(id="checker_log", wrap=True, highlight=True, markup=True)

            with TabPane("Outputs", id="outputs_tab"):
                with Horizontal():
                    with Vertical(classes="form-col"):
                        yield Label("steamid32.txt")
                        yield TextArea(id="out_steamid32", read_only=True)
                    with Vertical(classes="form-col"):
                        yield Label("goodaccounts.txt")
                        yield TextArea(id="out_goodaccounts", read_only=True)
                    with Vertical(classes="form-col"):
                        yield Label("badaccounts.txt")
                        yield TextArea(id="out_badaccounts", read_only=True)

        yield Footer()

    def on_mount(self) -> None:
        self.load_config()

    def load_config(self) -> None:
        saved_settings = paths.read_json_file('settings.json', {})
        if isinstance(saved_settings, dict):
            self.settings.update(saved_settings)
            
        self.query_one("#input_accounts", TextArea).text = paths.read_text_file('accounts.txt') or ""
        self.query_one("#input_rollids", TextArea).text = paths.read_text_file('rollids.txt') or ""
        self.query_one("#input_proxies", TextArea).text = paths.read_text_file('proxies.html') or ""

        # Map fields
        fields = [
            'default_nickname', 'default_profile_summary', 'default_custom_url',
            'profile_image_path', 'max_parallel_accounts', 'max_login_retries',
            'login_timeout_seconds', 'request_timeout_seconds'
        ]
        for f in fields:
            if f in self.settings:
                self.query_one(f"#{f}", Input).value = str(self.settings[f])

        if 'default_profile_theme' in self.settings:
            val = self.settings['default_profile_theme']
            try:
                self.query_one("#default_profile_theme", Select).value = val
            except Exception:
                pass

        switches = [
            'enable_namechange', 'enable_avatarchange', 'enable_descriptionchange',
            'enable_customurlchange', 'enable_themechange', 'enable_profile_verification',
            'enable_nameclear', 'enable_gatherid32', 'random_name', 'insert_random_chars_enabled', 'use_rollids'
        ]
        for s in switches:
            if s in self.settings:
                self.query_one(f"#{s}", Checkbox).value = bool(self.settings[s])

        self.update_outputs()

    def update_outputs(self) -> None:
        self.query_one("#out_steamid32", TextArea).text = paths.read_text_file('steamid32.txt') or ""
        self.query_one("#out_goodaccounts", TextArea).text = paths.read_text_file('goodaccounts.txt') or ""
        self.query_one("#out_badaccounts", TextArea).text = paths.read_text_file('badaccounts.txt') or ""

    def collect_settings(self) -> dict:
        s = {}
        fields = [
            'default_nickname', 'default_profile_summary', 'default_custom_url',
            'profile_image_path'
        ]
        for f in fields:
            s[f] = self.query_one(f"#{f}", Input).value

        num_fields = [
            'max_parallel_accounts', 'max_login_retries',
            'login_timeout_seconds', 'request_timeout_seconds'
        ]
        for f in num_fields:
            try:
                s[f] = int(self.query_one(f"#{f}", Input).value)
            except ValueError:
                pass

        s['default_profile_theme'] = self.query_one("#default_profile_theme", Select).value

        switches = [
            'enable_namechange', 'enable_avatarchange', 'enable_descriptionchange',
            'enable_customurlchange', 'enable_themechange', 'enable_profile_verification',
            'enable_nameclear', 'enable_gatherid32', 'random_name', 'insert_random_chars_enabled', 'use_rollids'
        ]
        for sw in switches:
            s[sw] = self.query_one(f"#{sw}", Checkbox).value

        return s

    def save_files(self) -> None:
        accs = self.query_one("#input_accounts", TextArea).text
        paths.write_text_file('accounts.txt', accs.strip() + '\n' if accs.strip() else '')
        
        pxs = self.query_one("#input_proxies", TextArea).text
        if pxs.strip():
            paths.write_text_file('proxies.html', pxs.strip() + '\n')
            
        rls = self.query_one("#input_rollids", TextArea).text
        if rls.strip():
            paths.write_text_file('rollids.txt', rls.strip() + '\n')
            
        settings = self.collect_settings()
        paths.write_json_file('settings.json', settings)
        self.settings.update(settings)

    def on_button_pressed(self, event: Button.Pressed) -> None:
        button_id = event.button.id
        
        if button_id == "btn_save":
            self.save_files()
            self.notify("Settings saved!")
            
        elif button_id == "btn_start":
            self.save_files()
            self.query_one("#btn_start", Button).disabled = True
            self.query_one("#btn_stop", Button).disabled = False
            self.query_one("#main_log", RichLog).clear()
            
            self.stop_event = threading.Event()
            s_obj = settings_from_dict(self.settings)
            
            def log_cb(line):
                self.call_from_thread(self.query_one("#main_log", RichLog).write, line)
                
            def worker():
                run_profile_update(s_obj, log_cb, self.stop_event)
                self.call_from_thread(self.on_job_finish)
                
            self.active_thread = threading.Thread(target=worker, daemon=True)
            self.active_thread.start()
            
        elif button_id == "btn_stop":
            if self.stop_event:
                self.stop_event.set()
                self.query_one("#main_log", RichLog).write("[bold red]Stop requested...[/]")
                
        elif button_id == "btn_start_check":
            self.save_files()
            self.query_one("#btn_start_check", Button).disabled = True
            self.query_one("#btn_stop_check", Button).disabled = False
            self.query_one("#checker_log", RichLog).clear()
            
            self.stop_event = threading.Event()
            s_obj = settings_from_dict(self.settings)
            
            def log_cb(line):
                self.call_from_thread(self.query_one("#checker_log", RichLog).write, line)
                
            def worker():
                run_account_checker(s_obj, log_cb, self.stop_event)
                self.call_from_thread(self.on_checker_finish)
                
            self.active_thread = threading.Thread(target=worker, daemon=True)
            self.active_thread.start()
            
        elif button_id == "btn_stop_check":
            if self.stop_event:
                self.stop_event.set()
                self.query_one("#checker_log", RichLog).write("[bold red]Stop requested...[/]")

    def on_job_finish(self) -> None:
        self.query_one("#btn_start", Button).disabled = False
        self.query_one("#btn_stop", Button).disabled = True
        self.update_outputs()
        self.active_thread = None
        self.stop_event = None

    def on_checker_finish(self) -> None:
        self.query_one("#btn_start_check", Button).disabled = False
        self.query_one("#btn_stop_check", Button).disabled = True
        self.update_outputs()
        self.active_thread = None
        self.stop_event = None

    def action_save(self) -> None:
        self.save_files()
        self.notify("Settings saved via hotkey!")

    def action_quit(self) -> None:
        self.exit()

def run_tui():
    app = AutoprofileTUI()
    app.run()

if __name__ == "__main__":
    run_tui()
