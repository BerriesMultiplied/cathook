const fields = [
  'default_nickname',
  'default_profile_summary',
  'default_custom_url',
  'default_profile_theme',
  'profile_image_path',
  'max_parallel_accounts',
  'max_login_retries',
  'login_timeout_seconds',
  'request_timeout_seconds',
  'loop_timeout'
];

const switches = [
  'enable_namechange',
  'enable_avatarchange',
  'enable_descriptionchange',
  'enable_customurlchange',
  'enable_themechange',
  'enable_profile_verification',
  'enable_nameclear',
  'enable_gatherid32',
  'random_name',
  'insert_random_chars_enabled',
  'use_rollids',
  'loopupdateprofiles'
];

function element(id) {
  return document.getElementById(id);
}

function autoResizeTextarea(textarea) {
  if (textarea.tagName === 'TEXTAREA') {
    textarea.style.height = 'auto';
    textarea.style.height = (textarea.scrollHeight) + 'px';
  }
}

async function request_json(url, options) {
  console.debug(`[DEBUG] fetch(${url})`, options || {});
  const response = await fetch(url, options);
  if (!response.ok) {
    const text = await response.text();
    console.error(`[ERROR] fetch(${url}) failed: ${response.status} ${response.statusText}`, text);
    throw new Error(text || response.statusText);
  }
  const json = await response.json();
  console.debug(`[DEBUG] fetch(${url}) response:`, json);
  return json;
}

function collect_settings() {
  const settings = {};
  for (const id of fields) {
    const input = element(id);
    settings[id] = input.type === 'number' ? Number(input.value) : input.value;
  }
  for (const id of switches) {
    settings[id] = element(id).checked;
  }
  return settings;
}

function apply_settings(settings) {
  for (const id of fields) {
    if (settings[id] !== undefined && element(id)) {
      element(id).value = settings[id];
    }
  }
  for (const id of switches) {
    if (settings[id] !== undefined && element(id)) {
      element(id).checked = Boolean(settings[id]);
    }
  }
}

async function load_config() {
  console.info('[INFO] load_config: Loading config from backend...');
  const config = await request_json('/api/config');
  element('accounts').value = config.accounts || '';
  element('proxies').value = config.proxies || '';
  if (element('rollids')) element('rollids').value = config.rollids || '';
  apply_settings(config.settings || {});
  element('steamid32_output').textContent = config.outputs?.steamid32 || '';
  element('goodaccounts_output').textContent = config.outputs?.goodaccounts || '';
  
  document.querySelectorAll('textarea').forEach(autoResizeTextarea);
  
  console.info('[INFO] load_config: Config applied to UI successfully.');
}

async function start_job() {
  console.info('[INFO] start_job: Preparing to start a new profile update job...');
  const payload = {
    accounts: element('accounts').value,
    proxies: element('proxies').value,
    rollids: element('rollids') ? element('rollids').value : '',
    settings: collect_settings()
  };
  await request_json('/api/profile-update/start', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload)
  });
  console.info('[INFO] start_job: Job started successfully. Polling status...');
  await poll_status();
}

async function save_files() {
  console.info('[INFO] save_files: Preparing to save current config to backend...');
  const payload = {
    accounts: element('accounts').value,
    proxies: element('proxies').value,
    rollids: element('rollids') ? element('rollids').value : '',
    settings: collect_settings()
  };
  await request_json('/api/config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload)
  });
  console.info('[INFO] save_files: Backend save successful. Reloading config...');
  await load_config();
  console.info('[INFO] save_files: Process complete.');

  const saveBtn = element('save_button');
  const originalHtml = saveBtn.innerHTML;
  saveBtn.innerHTML = '<i class="fa-solid fa-check"></i> saved!';
  saveBtn.style.color = '#a6da95';
  setTimeout(() => {
    saveBtn.innerHTML = originalHtml;
    saveBtn.style.color = '';
  }, 2000);
}

async function stop_job() {
  console.info('[INFO] stop_job: Sending stop request to backend...');
  await request_json('/api/profile-update/stop', {method: 'POST'});
  console.info('[INFO] stop_job: Stop request acknowledged.');
  await poll_status();
}

async function start_checker_job() {
  console.info('[INFO] start_checker_job: Preparing to start account checker job...');
  const payload = {
    accounts: element('accounts').value,
    proxies: element('proxies').value,
    settings: collect_settings()
  };
  await request_json('/api/account-check/start', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload)
  });
  console.info('[INFO] start_checker_job: Job started successfully. Polling status...');
  await poll_status();
}

async function stop_checker_job() {
  console.info('[INFO] stop_checker_job: Sending stop request to backend...');
  await request_json('/api/account-check/stop', {method: 'POST'});
  console.info('[INFO] stop_checker_job: Stop request acknowledged.');
  await poll_status();
}

function render_status(status) {
  element('job_status').textContent = status.status || 'idle';
  element('job_id').textContent = status.job_id ? status.job_id.slice(0, 8) : '';
  const escapeHTML = str => str.replace(/[&<>'"]/g, 
    tag => ({
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      "'": '&#39;',
      '"': '&quot;'
    }[tag] || tag)
  );
  
  const logs = status.logs || [];
  const formattedLogs = logs.map(item => {
    let line = escapeHTML(`[${item.time}] ${item.line}`);
    line = line.replace(/⚠/g, '<i class="fa-solid fa-triangle-exclamation" style="color: #f39c12;"></i>');
    line = line.replace(/✓/g, '<i class="fa-solid fa-check" style="color: #2ecc71;"></i>');
    return line;
  }).join('\n');
  element('logs').innerHTML = formattedLogs;
  element('log_count').textContent = `${logs.length} lines`;
  element('steamid32_output').textContent = status.outputs?.steamid32 || '';
  element('goodaccounts_output').textContent = status.outputs?.goodaccounts || '';
  element('start_button').disabled = status.running;
  element('stop_button').disabled = !status.running;
}

function render_checker_status(status) {
  const escapeHTML = str => str.replace(/[&<>'"]/g, 
    tag => ({
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      "'": '&#39;',
      '"': '&quot;'
    }[tag] || tag)
  );
  
  const logs = status.logs || [];
  const formattedLogs = logs.map(item => {
    let line = escapeHTML(`[${item.time}] ${item.line}`);
    line = line.replace(/⚠/g, '<i class="fa-solid fa-triangle-exclamation" style="color: #f39c12;"></i>');
    line = line.replace(/✓/g, '<i class="fa-solid fa-check" style="color: #2ecc71;"></i>');
    return line;
  }).join('\n');
  element('checker_logs').innerHTML = formattedLogs;
  element('checker_log_count').textContent = `${logs.length} lines`;
  element('badaccounts_output').textContent = status.outputs?.badaccounts || '';
  element('checker_start_button').disabled = status.running;
  element('checker_stop_button').disabled = !status.running;
}

async function poll_status() {
  const status = await request_json('/api/profile-update/status');
  render_status(status);
  
  const checker_status = await request_json('/api/account-check/status');
  render_checker_status(checker_status);
}

function setup_tabs() {
  console.info('[INFO] setup_tabs: Initializing navigation events...');
  for (const button of document.querySelectorAll('.nav_button')) {
    button.addEventListener('click', () => {
      console.debug(`[DEBUG] Tab clicked: ${button.dataset.panel}`);
      for (const item of document.querySelectorAll('.nav_button')) item.classList.remove('active');
      for (const panel of document.querySelectorAll('.panel')) panel.classList.remove('active');
      button.classList.add('active');
      element(button.dataset.panel).classList.add('active');
    });
  }
}

function setup_textareas() {
  document.querySelectorAll('textarea').forEach(textarea => {
    textarea.addEventListener('input', () => autoResizeTextarea(textarea));
  });
}

console.info('[INFO] Application initializing...');
element('start_button').addEventListener('click', () => {
  console.debug('[DEBUG] Start button clicked');
  start_job().catch(error => {
    console.error('[ERROR] start_job failed:', error);
    alert(error.message);
  });
});
element('stop_button').addEventListener('click', () => {
  console.debug('[DEBUG] Stop button clicked');
  stop_job().catch(error => {
    console.error('[ERROR] stop_job failed:', error);
    alert(error.message);
  });
});
element('checker_start_button').addEventListener('click', () => {
  console.debug('[DEBUG] Checker Start button clicked');
  start_checker_job().catch(error => {
    console.error('[ERROR] start_checker_job failed:', error);
    alert(error.message);
  });
});
element('checker_stop_button').addEventListener('click', () => {
  console.debug('[DEBUG] Checker Stop button clicked');
  stop_checker_job().catch(error => {
    console.error('[ERROR] stop_checker_job failed:', error);
    alert(error.message);
  });
});
element('save_button').addEventListener('click', () => {
  console.debug('[DEBUG] Save button clicked');
  save_files().catch(error => {
    console.error('[ERROR] save_files failed:', error);
    alert(error.message);
  });
});
setup_tabs();
setup_textareas();
load_config().then(() => {
  console.info('[INFO] Initial config loaded. Starting status poller...');
  return poll_status();
}).catch(error => {
  console.error('[ERROR] Initial load failed:', error);
  alert(error.message);
});
setInterval(() => poll_status().catch(err => console.debug('[DEBUG] Poller error (ignoring):', err)), 1500);
