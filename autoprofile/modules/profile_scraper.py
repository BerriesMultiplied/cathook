import re
import json
import random
from dataclasses import dataclass
from typing import Optional, List, Tuple
from autoprofile.core import paths

try:
    import requests
except ImportError:
    requests = None

@dataclass
class ScrapedProfile:
    steamid64: str
    persona_name: Optional[str]
    summary: Optional[str]
    avatar_url: Optional[str]
    avatar_bytes: Optional[bytes]
    theme_id: Optional[str]
    custom_url: Optional[str]

class ProfileScraper:
    def __init__(self, rollids_path: str = 'rollids.txt'):
        self.rollids_path = rollids_path
        self.usage_counts = {}
        self.load_ids()

    def load_ids(self):
        lines = paths.read_lines_file(self.rollids_path)
        for line in lines:
            line = line.strip()
            if not line:
                continue
            id64 = self._resolve_steamid64(line)
            if id64 and id64 not in self.usage_counts:
                self.usage_counts[id64] = 0

    def get_least_used_id(self) -> Optional[str]:
        if not self.usage_counts:
            return None
        return min(self.usage_counts.items(), key=lambda x: x[1])[0]

    def record_usage(self, id64: str):
        if id64 in self.usage_counts:
            self.usage_counts[id64] += 1

    def scrape_random_profile(self) -> Optional[ScrapedProfile]:
        id64 = self.get_least_used_id()
        if not id64:
            return None
        
        profile = self._scrape_profile(id64)
        if profile:
            self.record_usage(id64)
            return profile
        return None

    def _resolve_steamid64(self, steam_id: str) -> Optional[str]:
        if steam_id.isdigit() and len(steam_id) == 17:
            return steam_id
        
        if steam_id.isdigit():
            steamid32 = int(steam_id)
            steamid64 = steamid32 + 76561197960265728
            return str(steamid64)
        
        if requests is None:
            return None
            
        try:
            resp = requests.get(f"https://steamcommunity.com/id/{steam_id}/?xml=1", timeout=10)
            if resp.status_code == 200:
                match = re.search(r'<steamID64>(\d+)</steamID64>', resp.text)
                if match:
                    return match.group(1)
        except Exception:
            pass
        return None

    def _scrape_profile(self, id64: str) -> Optional[ScrapedProfile]:
        if requests is None:
            return None
        
        url = f"https://steamcommunity.com/profiles/{id64}"
        try:
            resp = requests.get(url, timeout=15)
            if resp.status_code != 200:
                return None
            
            html = resp.text
            
            persona_name = None
            name_match = re.search(r'<span class="actual_persona_name">([^<]+)</span>', html)
            if name_match:
                persona_name = name_match.group(1).strip()
                
            summary = None
            summary_match = re.search(r'<div class="profile_summary"[^>]*>(.*?)</div>', html, re.DOTALL)
            if summary_match:
                raw_summary = summary_match.group(1)
                summary = re.sub(r'<br\s*/?>', '\n', raw_summary)
                summary = re.sub(r'<[^>]+>', '', summary).strip()
                
            avatar_url = None
            avatar_bytes = None
            avatar_match = re.search(r'https://avatars\.[^"\'>\s]+_full\.jpg', html)
            if avatar_match:
                avatar_url = avatar_match.group(0)
                try:
                    img_resp = requests.get(avatar_url, timeout=15)
                    if img_resp.status_code == 200:
                        avatar_bytes = img_resp.content
                except Exception:
                    pass

            theme_id = None
            theme_match = re.search(r'"theme_id":"([^"]+)"', html)
            free_themes = ['Summer', 'Midnight', 'Steel', 'Cosmic', 'DarkMode']
            if theme_match and theme_match.group(1) in free_themes:
                theme_id = theme_match.group(1)
            else:
                theme_id = random.choice(free_themes)
                
            custom_url = None
            url_match = re.search(r'https://steamcommunity\.com/id/([^/"]+)/?', html)
            import string
            random_suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=4))
            
            if url_match:
                original_custom_url = url_match.group(1)
                custom_url = f"{original_custom_url}{random_suffix}"
            elif persona_name:
                base_url = re.sub(r'[^a-zA-Z0-9]', '', persona_name).lower()
                if base_url:
                    custom_url = f"{base_url}{random_suffix}"
                
            return ScrapedProfile(
                steamid64=id64,
                persona_name=persona_name,
                summary=summary,
                avatar_url=avatar_url,
                avatar_bytes=avatar_bytes,
                theme_id=theme_id,
                custom_url=custom_url
            )
        except Exception as e:
            print(f"Error scraping profile {id64}: {e}")
            return None
