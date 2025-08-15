/* eslint-disable comma-dangle */
/* eslint-disable max-len */
/* eslint-disable require-jsdoc */

document.addEventListener('DOMContentLoaded', init, false);
let colorTimer = undefined;
let colorUpdated  = false;
let storedModelId = 255;
let buttonActions = [];
let modeSelectionInit = true;
let originalUID = undefined;
let originalUIDType = undefined;

// Fallback if libs.js (cuteAlert) isn’t loaded yet:
if (typeof window.cuteAlert !== 'function') {
  window.cuteAlert = ({ type, title, message, confirmText, cancelText }) => {
    alert(`${title || type || 'Info'}\n\n${message || ''}`);
    return Promise.resolve('confirm');
  };
}

function _(el) {
  return document.getElementById(el);
}

function getPwmFormData() {
  let ch = 0;
  let inField;
  const outData = [];
  while (inField = _(`pwm_${ch}_ch`)) {
    const inChannel = inField.value;
    const mode = _(`pwm_${ch}_mode`).value;
    const invert = _(`pwm_${ch}_inv`).checked ? 1 : 0;
    const narrow = _(`pwm_${ch}_nar`).checked ? 1 : 0;
    const failsafeField = _(`pwm_${ch}_fs`);
    const failsafeModeField = _(`pwm_${ch}_fsmode`);
    let failsafe = failsafeField.value;
    if (failsafe > 2011) failsafe = 2011;
    if (failsafe < 988) failsafe = 988;
    failsafeField.value = failsafe;
    let failsafeMode = failsafeModeField.value;

    const raw = (narrow << 19) | (mode << 15) | (invert << 14) | (inChannel << 10) | (failsafeMode << 20) | (failsafe - 988);
    outData.push(raw);
    ++ch;
  }
  return outData;
}

function enumSelectGenerate(id, val, arOptions) {
  const retVal = `<div class="mui-select compact"><select id="${id}" class="pwmitm">` +
        arOptions.map((item, idx) => {
          if (item) return `<option value="${idx}"${(idx === val) ? ' selected' : ''} ${item === 'Disabled' ? 'disabled' : ''}>${item}</option>`;
          return '';
        }).join('') + '</select></div>';
  return retVal;
}

function generateFeatureBadges(features) {
  let str = '';
  if (!!(features & 1)) str += `<span style="color: #696969; background-color: #a8dcfa" class="badge">TX</span>`;
  else if (!!(features & 2)) str += `<span style="color: #696969; background-color: #d2faa8" class="badge">RX</span>`;
  if ((features & 12) === 12) str += `<span style="color: #696969; background-color: #fab4a8" class="badge">I2C</span>`;
  else if (!!(features & 4)) str += `<span style="color: #696969; background-color: #fab4a8" class="badge">SCL</span>`;
  else if (!!(features & 8)) str += `<span style="color: #696969; background-color: #fab4a8" class="badge">SDA</span>`;

  if ((features & 96) === 96) str += `<span style="color: #696969; background-color: #36b5ff" class="badge">Serial2</span>`;
  else if (!!(features & 32)) str += `<span style="color: #696969; background-color: #36b5ff" class="badge">RX2</span>`;
  else if (!!(features & 64)) str += `<span style="color: #696969; background-color: #36b5ff" class="badge">TX2</span>`;

  return str;
}

function updatePwmSettings(arPwm) {
  // Safe no-op if PWM UI isn’t present for your device
  if (!arPwm || !_('pwm')) {
    if (_('model_tab')) _('model_tab').style.display = 'none';
    return;
  }
  var pinRxIndex = undefined;
  var pinTxIndex = undefined;
  var pinModes = [];
  const htmlFields = ['<div class="mui-panel pwmpnl"><table class="pwmtbl mui-table"><tr><th class="fixed-column">Output</th><th class="mui--text-center fixed-column">Features</th><th>Mode</th><th>Input</th><th class="mui--text-center fixed-column">Invert?</th><th class="mui--text-center fixed-column">750us?</th><th class="mui--text-center fixed-column pwmitm">Failsafe Mode</th><th class="mui--text-center fixed-column pwmitm">Failsafe Pos</th></tr>'];
  arPwm.forEach((item, index) => {
    const failsafe = (item.config & 1023) + 988;
    const failsafeMode = (item.config >> 20) & 3;
    const ch = (item.config >> 10) & 15;
    const inv = (item.config >> 14) & 1;
    const mode = (item.config >> 15) & 15;
    const narrow = (item.config >> 19) & 1;
    const features = item.features;
    const modes = ['50Hz', '60Hz', '100Hz', '160Hz', '333Hz', '400Hz', '10KHzDuty', 'On/Off'];
    if (features & 16) {
      modes.push('DShot');
    } else {
      modes.push(undefined);
    }
    if (features & 1) {
      modes.push('Serial TX'); modes.push(undefined); modes.push(undefined); modes.push(undefined);
      pinRxIndex = index;
    } else if (features & 2) {
      modes.push('Serial RX'); modes.push(undefined); modes.push(undefined); modes.push(undefined);
      pinTxIndex = index;
    } else {
      modes.push(undefined);
      if (features & 4) modes.push('I2C SCL'); else modes.push(undefined);
      if (features & 8) modes.push('I2C SDA'); else modes.push(undefined);
      modes.push(undefined);
    }

    if (features & 32) modes.push('Serial2 RX'); else modes.push(undefined);
    if (features & 64) modes.push('Serial2 TX'); else modes.push(undefined);

    const modeSelect = enumSelectGenerate(`pwm_${index}_mode`, mode, modes);
    const inputSelect = enumSelectGenerate(`pwm_${index}_ch`, ch,
        ['ch1','ch2','ch3','ch4','ch5 (AUX1)','ch6 (AUX2)','ch7 (AUX3)','ch8 (AUX4)','ch9 (AUX5)','ch10 (AUX6)','ch11 (AUX7)','ch12 (AUX8)','ch13 (AUX9)','ch14 (AUX10)','ch15 (AUX11)','ch16 (AUX12)']);
    const failsafeModeSelect = enumSelectGenerate(`pwm_${index}_fsmode`, failsafeMode, ['Set Position','No Pulses','Last Position']);

    htmlFields.push(`<tr><td class="mui--text-center mui--text-title">${index + 1}</td>
            <td>${generateFeatureBadges(features)}</td>
            <td>${modeSelect}</td>
            <td>${inputSelect}</td>
            <td><div class="mui-checkbox mui--text-center"><input type="checkbox" id="pwm_${index}_inv"${(inv) ? ' checked' : ''}></div></td>
            <td><div class="mui-checkbox mui--text-center"><input type="checkbox" id="pwm_${index}_nar"${(narrow) ? ' checked' : ''}></div></td>
            <td>${failsafeModeSelect}</td>
            <td><div class="mui-textfield compact"><input id="pwm_${index}_fs" value="${failsafe}" size="6" class="pwmitm" /></div></td></tr>`);
    pinModes[index] = mode;
  });
  htmlFields.push('</table></div>');

  const grp = document.createElement('DIV');
  grp.setAttribute('class', 'group');
  grp.innerHTML = htmlFields.join('');
  _('pwm').appendChild(grp);

  const setDisabled = (index, onoff) => {
    _(`pwm_${index}_ch`).disabled = onoff;
    _(`pwm_${index}_inv`).disabled = onoff;
    _(`pwm_${index}_nar`).disabled = onoff;
    _(`pwm_${index}_fs`).disabled = onoff;
    _(`pwm_${index}_fsmode`).disabled = onoff;
  };

  arPwm.forEach((item,index)=>{
    const pinMode = _(`pwm_${index}_mode`);
    pinMode.onchange = () => {
      setDisabled(index, pinMode.value > 9);
      const updateOthers = (value, enable) => {
        if (value > 9) {
          arPwm.forEach((_item, other) => {
            if (other != index) {
              document.querySelectorAll(`#pwm_${other}_mode option`).forEach(opt => {
                if (opt.value == value) {
                  if (modeSelectionInit) opt.disabled = true;
                  else opt.disabled = enable;
                }
              });
            }
          })
        }
      }
      updateOthers(pinMode.value, true);
      updateOthers(pinModes[index], false);
      pinModes[index] = pinMode.value;

      if (_('serial1-config')) {
        _('serial1-config').style.display = 'none';
        if (pinMode.value == 14) _('serial1-config').style.display = 'block';
      }
    };
    pinMode.onchange();

    const failsafeMode = _(`pwm_${index}_fsmode`);
    failsafeMode.onchange = () => {
      const failsafeField = _(`pwm_${index}_fs`);
      if (failsafeMode.value == 0) { failsafeField.disabled = false; failsafeField.style.display = 'block'; }
      else { failsafeField.disabled = true; failsafeField.style.display = 'none'; }
    };
    failsafeMode.onchange();
  });

  modeSelectionInit = false;

  // pin Rx/Tx constraints (if present)
  if (pinRxIndex !== undefined && pinTxIndex !== undefined) {
    const pinRxMode = _(`pwm_${pinRxIndex}_mode`);
    const pinTxMode = _(`pwm_${pinTxIndex}_mode`);
    const serialCfg = _('serial-config');
    const baudCfg   = _('baud-config');

    pinRxMode.onchange = () => {
      if (pinRxMode.value == 9) {
        pinTxMode.value = 9;
        setDisabled(pinRxIndex, true);
        setDisabled(pinTxIndex, true);
        pinTxMode.disabled = true;
        if (serialCfg) serialCfg.style.display = 'block';
        if (baudCfg)   baudCfg.style.display = 'block';
      } else {
        pinTxMode.value = 0;
        setDisabled(pinRxIndex, false);
        setDisabled(pinTxIndex, false);
        pinTxMode.disabled = false;
        if (serialCfg) serialCfg.style.display = 'none';
        if (baudCfg)   baudCfg.style.display = 'none';
      }
    };
    pinTxMode.onchange = () => {
      if (pinTxMode.value == 9) {
        pinRxMode.value = 9;
        setDisabled(pinRxIndex, true);
        setDisabled(pinTxIndex, true);
        pinTxMode.disabled = true;
        if (serialCfg) serialCfg.style.display = 'block';
        if (baudCfg)   baudCfg.style.display = 'block';
      }
    };
    const pinTx = pinTxMode.value;
    pinRxMode.onchange();
    if (pinRxMode.value != 9) pinTxMode.value = pinTx;
  }
}

function init() {
  // network radio buttons (only if present)
  if (_('nt0') && _('nt1') && _('nt2') && _('nt3')) {
    _('nt0').onclick = () => _('credentials').style.display = 'block';
    _('nt1').onclick = () => _('credentials').style.display = 'block';
    _('nt2').onclick = () => _('credentials').style.display = 'none';
    _('nt3').onclick = () => _('credentials').style.display = 'none';
  }

  // Start on the Options tab if available, otherwise do nothing
  try { if (window.mui && mui.tabs) mui.tabs.activate('pane-justified-1'); } catch(e) {}

  initFiledrag();
  initOptions();

  // lazy network scan on WiFi tab show (if that tab exists)
  const netTab = _('network-tab');
  if (netTab && netTab.addEventListener) {
    netTab.addEventListener('mui.tabs.showstart', getNetworks);
  }
}

function updateUIDType(uidtype) {
  let bg = '';
  let fg = 'white';
  let desc = '';

  if (!uidtype || uidtype.startsWith('Not set')) {
    bg = '#D50000'; uidtype = 'Not set'; desc = 'Using autogenerated binding UID';
  }
  else if (uidtype === 'Flashed') { bg = '#1976D2'; desc = 'The binding UID was generated from a binding phrase set at flash time'; }
  else if (uidtype === 'Overridden') { bg = '#689F38'; fg = 'black'; desc = 'The binding UID has been generated from a binding phrase previously entered'; }
  else if (uidtype === 'Modified') { bg = '#7c00d5'; desc = 'The binding UID has been modified, but not yet saved'; }
  else if (uidtype === 'Volatile') { bg = '#FFA000'; desc = 'The binding UID will be cleared on boot'; }
  else if (uidtype === 'Loaned') { bg = '#FFA000'; desc = 'This receiver is on loan and can be returned using Lua or three-plug'; }
  else {
    if (_('uid') && _('uid').value.endsWith('0,0,0,0')) { bg = '#FFA000'; uidtype = 'Not bound'; desc = 'This receiver is unbound and will boot to binding mode'; }
    else { bg = '#1976D2'; uidtype = 'Bound'; desc = 'This receiver is bound and will boot waiting for connection'; }
  }

  if (_('uid-type')) {
    _('uid-type').style.backgroundColor = bg;
    _('uid-type').style.color = fg;
    _('uid-type').textContent = uidtype;
  }
  if (_('uid-text')) _('uid-text').textContent = desc;
}

function updateConfig(data, options) {
  if (data.product_name && _('product_name')) _('product_name').textContent = data.product_name;
  if (data.reg_domain && _('reg_domain')) _('reg_domain').textContent = data.reg_domain;
  if (data.uid && _('uid')) {
    _('uid').value = data.uid.toString();
    originalUID = data.uid;
  }
  originalUIDType = data.uidtype;
  updateUIDType(data.uidtype);

  if (data.mode === 'STA') {
    if (_('stamode')) _('stamode').style.display = 'block';
    if (_('ssid')) _('ssid').textContent = data.ssid || '';
  } else {
    if (_('apmode')) _('apmode').style.display = 'block';
  }

  // RX-ish config (optional if DOM exists)
  if (_('model-match')) {
    _('model-match').onclick = () => {
      if (_('model-match').checked) {
        if (_('modelNum')) _('modelNum').style.display = 'block';
        if (storedModelId === 255) { if (_('modelid')) _('modelid').value = ''; }
        else { if (_('modelid')) _('modelid').value = storedModelId; }
      } else {
        if (_('modelNum')) _('modelNum').style.display = 'none';
        if (_('modelid')) _('modelid').value = '255';
      }
    };
  }

  if ('modelid' in data && data.modelid !== 255) {
    if (_('modelNum')) _('modelNum').style.display = 'block';
    if (_('model-match')) _('model-match').checked = true;
    storedModelId = data.modelid;
  } else {
    if (_('modelNum')) _('modelNum').style.display = 'none';
    if (_('model-match')) _('model-match').checked = false;
    storedModelId = 255;
  }
  if (_('modelid')) _('modelid').value = storedModelId;

  if (_('force-tlm')) _('force-tlm').checked = data.hasOwnProperty('force-tlm') && data['force-tlm'];

  if (_('serial-protocol')) {
    _('serial-protocol').onchange = () => {
      const proto = Number(_('serial-protocol').value);
      const isAirport = _('is-airport') && _('is-airport').checked;
      const baud = _('rcvr-uart-baud');
      const sCfg = _('serial-config');
      const sBus = _('sbus-config');

      if (isAirport) {
        if (baud) { baud.disabled = false; baud.value = options['rcvr-uart-baud']; }
        if (sCfg) sCfg.style.display = 'none';
        if (sBus) sBus.style.display = 'none';
        return;
      }
      if (sCfg) sCfg.style.display = 'block';

      if (proto === 0 || proto === 1) {
        if (baud) { baud.disabled = false; baud.value = options['rcvr-uart-baud']; }
        if (sBus) sBus.style.display = 'none';
      }
      else if (proto === 2 || proto === 3 || proto === 5) {
        if (baud) { baud.disabled = true; baud.value = '100000'; }
        if (sBus) {
          sBus.style.display = 'block';
          if (_('sbus-failsafe') && data['sbus-failsafe'] != null) _('sbus-failsafe').value = data['sbus-failsafe'];
        }
      }
      else if (proto === 4) {
        if (baud) { baud.disabled = true; baud.value = '115200'; }
        if (sBus) sBus.style.display = 'none';
      }
      else if (proto === 6) {
        if (baud) { baud.disabled = true; baud.value = '19200'; }
        if (sBus) sBus.style.display = 'none';
      }
    };
  }

  if (_('serial1-protocol')) {
    _('serial1-protocol').onchange = () => {
      const isAirport = _('is-airport') && _('is-airport').checked;
      const baud = _('rcvr-uart-baud');
      const s1Cfg = _('serial1-config');
      const sBus = _('sbus-config');
      if (isAirport) {
        if (baud) { baud.disabled = false; baud.value = options['rcvr-uart-baud']; }
        if (s1Cfg) s1Cfg.style.display = 'none';
        if (sBus) sBus.style.display = 'none';
      }
    };
  }

  updatePwmSettings(data.pwm);

  if (_('serial-protocol')) { _('serial-protocol').value = data['serial-protocol']; _('serial-protocol').onchange(); }
  if (_('serial1-protocol')) { _('serial1-protocol').value = data['serial1-protocol']; _('serial1-protocol').onchange(); }
  if (_('is-airport')) {
    _('is-airport').onchange = () => {
      if (_('serial-protocol')) _('serial-protocol').onchange();
      if (_('serial1-protocol')) _('serial1-protocol').onchange();
    };
  }
  if (_('vbind')) {
    _('vbind').value = data.vbind;
    _('vbind').onchange = () => {
      if (_('bindphrase')) _('bindphrase').style.display = (_('vbind').value === '1' ? 'none' : 'block');
    };
    _('vbind').onchange();
  }

  if (_('serial1-config')) {
    _('serial1-config').style.display = 'none';
    data.pwm?.forEach((item,index) => {
      const _pinMode = _(`pwm_${index}_mode`);
      if (_pinMode && _pinMode.value == 14) _('serial1-config').style.display = 'block';
    });
  }
}

function initOptions() {
  const xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange = function() {
    if (this.readyState === 4 && this.status === 200) {
      const data = JSON.parse(this.responseText);
      updateOptions(data['options']);
      updateConfig(data['config'], data['options']);
      initBindingPhraseGen();
    }
  };
  xmlhttp.open('GET', '/config', true);
  xmlhttp.send();
}

function getNetworks() {
  const xmlhttp = new XMLHttpRequest();
  xmlhttp.onload = function() {
    if (this.status === 204) {
      setTimeout(getNetworks, 2000);
    } else {
      try {
        const data = JSON.parse(this.responseText);
        if (data.length > 0) {
          if (_('loader')) _('loader').style.display = 'none';
          if (typeof autocomplete === 'function' && _('network')) autocomplete(_('network'), data);
        }
      } catch (_) {}
    }
  };
  xmlhttp.onerror = function() {
    setTimeout(getNetworks, 2000);
  };
  xmlhttp.open('GET', 'networks.json', true);
  xmlhttp.send();
}

// =========================================================

function initFiledrag() {
  const fileselect = _('firmware_file');
  const filedrag = _('filedrag');

  if (fileselect) fileselect.addEventListener('change', fileSelectHandler, false);

  const xhr = new XMLHttpRequest();
  if (xhr.upload && filedrag) {
    filedrag.addEventListener('dragover', fileDragHover, false);
    filedrag.addEventListener('dragleave', fileDragHover, false);
    filedrag.addEventListener('drop', fileSelectHandler, false);
    filedrag.style.display = 'block';
  }
}

function fileDragHover(e) {
  e.stopPropagation();
  e.preventDefault();
  if (e.target === _('filedrag')) e.target.className = (e.type === 'dragover' ? 'hover' : '');
}

function fileSelectHandler(e) {
  fileDragHover(e);
  const files = e.target.files || e.dataTransfer.files;
  if (!files || !files[0]) return;

  // ESP32 expects .bin
  const expectedFileExt = 'bin';
  const expectedFileExtDesc = '.bin file.';
  const fileExt = files[0].name.split('.').pop();

  if (fileExt === expectedFileExt) {
    uploadFile(files[0]);
  } else {
    cuteAlert({
      type: 'error',
      title: 'Incorrect File Format',
      message: 'You selected the file &quot;' + files[0].name.toString() + '&quot;.<br />The firmware file must be a ' + expectedFileExtDesc
    });
  }
}

function uploadFile(file) {
  const btn = _('upload_btn');
  if (btn) btn.disabled = true;
  try {
    const formdata = new FormData();
    formdata.append('upload', file, file.name);
    const ajax = new XMLHttpRequest();
    ajax.upload.addEventListener('progress', progressHandler, false);
    ajax.addEventListener('load', completeHandler, false);
    ajax.addEventListener('error', errorHandler, false);
    ajax.addEventListener('abort', abortHandler, false);
    ajax.open('POST', '/update');
    ajax.setRequestHeader('X-FileSize', file.size);
    ajax.send(formdata);
  } catch (e) {
    if (btn) btn.disabled = false;
  }
}

function progressHandler(event) {
  const percent = Math.round((event.loaded / event.total) * 100);
  if (_('progressBar')) _('progressBar').value = percent;
  if (_('status')) _('status').innerHTML = percent + '% uploaded... please wait';
}

function completeHandler(event) {
  if (_('status')) _('status').innerHTML = '';
  if (_('progressBar')) _('progressBar').value = 0;
  if (_('upload_btn')) _('upload_btn').disabled = false;

  let data = {};
  try { data = JSON.parse(event.target.responseText); } catch (_) {}

  if (data.status === 'ok') {
    function showMessage() {
      cuteAlert({ type: 'success', title: 'Update Succeeded', message: data.msg || '' });
    }
    let percent = 0;
    const interval = setInterval(()=>{
      percent = percent + 2;
      if (_('progressBar')) _('progressBar').value = percent;
      if (_('status')) _('status').innerHTML = percent + '% flashed... please wait';
      if (percent >= 100) {
        clearInterval(interval);
        if (_('status')) _('status').innerHTML = '';
        if (_('progressBar')) _('progressBar').value = 0;
        showMessage();
      }
    }, 100);
  } else if (data.status === 'mismatch') {
    cuteAlert({
      type: 'question',
      title: 'Targets Mismatch',
      message: data.msg || '',
      confirmText: 'Flash anyway',
      cancelText: 'Cancel'
    }).then((e)=>{
      const xmlhttp = new XMLHttpRequest();
      xmlhttp.onreadystatechange = function() {
        if (this.readyState === 4) {
          if (_('status')) _('status').innerHTML = '';
          if (_('progressBar')) _('progressBar').value = 0;
          if (this.status === 200) {
            const data = JSON.parse(this.responseText);
            cuteAlert({ type: 'info', title: 'Force Update', message: data.msg || '' });
          } else {
            cuteAlert({ type: 'error', title: 'Force Update', message: 'An error occurred trying to force the update' });
          }
        }
      };
      xmlhttp.open('POST', '/forceupdate', true);
      const fd = new FormData();
      fd.append('action', e);
      xmlhttp.send(fd);
    });
  } else {
    cuteAlert({ type: 'error', title: 'Update Failed', message: data.msg || event.target.responseText || '' });
  }
}

function errorHandler(event) {
  if (_('status')) _('status').innerHTML = '';
  if (_('progressBar')) _('progressBar').value = 0;
  if (_('upload_btn')) _('upload_btn').disabled = false;
  cuteAlert({ type: 'error', title: 'Update Failed', message: event.target && event.target.responseText || '' });
}

function abortHandler(event) {
  if (_('status')) _('status').innerHTML = '';
  if (_('progressBar')) _('progressBar').value = 0;
  if (_('upload_btn')) _('upload_btn').disabled = false;
  cuteAlert({ type: 'info', title: 'Update Aborted', message: event.target && event.target.responseText || '' });
}

// =========================================================

function callback(title, msg, url, getdata, success) {
  return function(e) {
    e.stopPropagation();
    e.preventDefault();
    const xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
      if (this.readyState === 4) {
        if (this.status === 200) {
          if (success) success();
          cuteAlert({ type: 'info', title: title, message: this.responseText });
        } else {
          cuteAlert({ type: 'error', title: title, message: msg });
        }
      }
    };
    xmlhttp.open('POST', url, true);
    const data = getdata ? getdata(xmlhttp) : null;
    xmlhttp.send(data);
  };
}

function setupNetwork(event) {
  if (_('nt0') && _('nt0').checked) {
    callback('Set Home Network', 'An error occurred setting the home network', '/sethome?save', function() {
      return new FormData(_('sethome'));
    }, function() {
      if (_('wifi-ssid')) _('wifi-ssid').value = _('network').value;
      if (_('wifi-password')) _('wifi-password').value = _('password').value;
    })(event);
  }
  if (_('nt1') && _('nt1').checked) {
    callback('Connect To Network', 'An error occurred connecting to the network', '/sethome', function() {
      return new FormData(_('sethome'));
    })(event);
  }
  if (_('nt2') && _('nt2').checked) {
    callback('Start Access Point', 'An error occurred starting the Access Point', '/access', null)(event);
  }
  if (_('nt3') && _('nt3').checked) {
    callback('Forget Home Network', 'An error occurred forgetting the home network', '/forget', null)(event);
  }
}

if (_('reset-model')) {
  _('reset-model').addEventListener('click', callback('Reset Model Settings', 'An error occurred reseting model settings', '/reset?model', null));
}
if (_('reset-options')) {
  _('reset-options').addEventListener('click', callback('Reset Runtime Options', 'An error occurred reseting runtime options', '/reset?options', null));
}

if (_('sethome')) _('sethome').addEventListener('submit', setupNetwork);
if (_('connect')) _('connect').addEventListener('click', callback('Connect to Home Network', 'An error occurred connecting to the Home network', '/connect', null));

if (_('config')) {
  _('config').addEventListener('submit', callback('Set Configuration', 'An error occurred updating the configuration', '/config',
      (xmlhttp) => {
        xmlhttp.setRequestHeader('Content-Type', 'application/json');
        return JSON.stringify({
          "pwm": getPwmFormData(),
          "serial-protocol": +(_('serial-protocol')?.value || 0),
          "serial1-protocol": +(_('serial1-protocol')?.value || 0),
          "sbus-failsafe": +(_('sbus-failsafe')?.value || 0),
          "modelid": +(_('modelid')?.value || 255),
          "force-tlm": !!_('force-tlm')?.checked,
          "vbind": +(_('vbind')?.value || 0),
          "uid": (_('uid')?.value || '').split(',').map(Number).filter(n=>!isNaN(n)),
        });
      }, () => {
        originalUID = _('uid')?.value;
        originalUIDType = 'Bound';
        if (_('phrase')) _('phrase').value = '';
        updateUIDType(originalUIDType);
    }));
}

function submitOptions(e) {
  e.stopPropagation();
  e.preventDefault();
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/options.json');
  xhr.setRequestHeader('Content-Type', 'application/json');

  const formElem = _('upload_options');
  const formObject = formElem ? Object.fromEntries(new FormData(formElem)) : {};
  if (formElem) {
    formElem.querySelectorAll('input[type=checkbox]:not(:checked)').forEach((k) => formObject[k.name] = false);
  }
  formObject['customised'] = true;

  xhr.send(JSON.stringify(formObject, function(k, v) {
    if (v === '') return undefined;
    if (_(k)) {
      if (_(k).type === 'color') return undefined;
      if (_(k).type === 'checkbox') return v === 'on';
      if (_(k).classList.contains && _(k).classList.contains('datatype-boolean')) return v === 'true';
      if (_(k).classList.contains && _(k).classList.contains('array')) {
        const arr = v.split(',').map((element) => Number(element));
        return arr.length === 0 ? undefined : arr;
      }
    }
    if (typeof v === 'boolean') return v;
    if (v === 'true') return true;
    if (v === 'false') return false;
    return isNaN(v) ? v : +v;
  }));

  xhr.onreadystatechange = function() {
    if (this.readyState === 4) {
      if (this.status === 200) {
        cuteAlert({
          type: 'question',
          title: 'Upload Succeeded',
          message: 'Reboot to take effect',
          confirmText: 'Reboot',
          cancelText: 'Close'
        }).then((e) => {
          if (e === 'confirm') {
            const xhr2 = new XMLHttpRequest();
            xhr2.open('POST', '/reboot');
            xhr2.setRequestHeader('Content-Type', 'application/json');
            xhr2.onreadystatechange = function() {};
            xhr2.send();
          }
        });
      } else {
        cuteAlert({
          type: 'error',
          title: 'Upload Failed',
          message: this.responseText
        });
      }
    }
  };
}

if (_('submit-options')) _('submit-options').addEventListener('click', submitOptions);

// ===== Button color helpers (guarded; only used if DOM has those controls) =====
function toRGB(c) {
  let r = c & 0xE0; r = ((r << 16) + (r << 13) + (r << 10)) & 0xFF0000;
  let g = c & 0x1C; g = ((g << 11) + (g << 8) + (g << 5)) & 0xFF00;
  let b = ((c & 0x3) << 1) + ((c & 0x3) >> 1); b = (b << 5) + (b << 2) + (b >> 1);
  const s = (r+g+b).toString(16);
  return '#' + "000000".substring(0, 6-s.length) + s;
}

function updateButtons(data) {
  buttonActions = data;
  if (!data) return;
  for (const [b, _v] of Object.entries(data)) {
    for (const [p, v] of Object.entries(_v['action'])) {
      appendRow(parseInt(b), parseInt(p), v);
    }
    if (_v['color'] !== undefined && _(`button${parseInt(b)+1}-color-div`)) {
      _(`button${parseInt(b)+1}-color-div`).style.display = 'block';
      if (_(`button${parseInt(b)+1}-color`)) _(`button${parseInt(b)+1}-color`).value = toRGB(_v['color']);
    }
  }
  if (_('button1-color')) _('button1-color').oninput = changeCurrentColors;
  if (_('button2-color')) _('button2-color').oninput = changeCurrentColors;
}

function changeCurrentColors() {
  if (colorTimer === undefined) {
    sendCurrentColors();
    colorTimer = setInterval(timeoutCurrentColors, 50);
  } else {
    colorUpdated = true;
  }
}

function to8bit(v) {
  v = parseInt(v.substring(1), 16);
  return ((v >> 16) & 0xE0) + ((v >> (8+3)) & 0x1C) + ((v >> 6) & 0x3);
}

function sendCurrentColors() {
  const form = _('button_actions');
  if (!form) return;
  const formData = new FormData(form);
  const data = Object.fromEntries(formData);
  const colors = [];
  for (const [k, v] of Object.entries(data)) {
    if (_(k) && _(k).type === 'color') {
      const index = parseInt(k.substring('6')) - 1;
      if (_(k + '-div') && _(k + '-div').style.display === 'none') colors[index] = -1;
      else colors[index] = to8bit(v);
    }
  }
  const xmlhttp = new XMLHttpRequest();
  xmlhttp.open('POST', '/buttons', true);
  xmlhttp.setRequestHeader('Content-type', 'application/json');
  xmlhttp.send(JSON.stringify(colors));
  colorUpdated = false;
}

function timeoutCurrentColors() {
  if (colorUpdated) sendCurrentColors();
  else { clearInterval(colorTimer); colorTimer = undefined; }
}

function checkEnableButtonActionSave() {
  let disable = false;
  for (const [b, _v] of Object.entries(buttonActions || {})) {
    for (const [p, v] of Object.entries(_v['action'])) {
      if (v['action'] !== 0 && (_(`select-press-${b}-${p}`)?.value === '' || _(`select-long-${b}-${p}`)?.value === '' || _(`select-short-${b}-${p}`)?.value === '')) {
        disable = true;
      }
    }
  }
  if (_('submit-actions')) _('submit-actions').disabled = disable;
}

function changeAction(b, p, value) {
  if (!buttonActions[b]) return;
  buttonActions[b]['action'][p]['action'] = value;
  if (value === 0) {
    if (_(`select-press-${b}-${p}`)) _(`select-press-${b}-${p}`).value = '';
    if (_(`select-long-${b}-${p}`)) _(`select-long-${b}-${p}`).value = '';
    if (_(`select-short-${b}-${p}`)) _(`select-short-${b}-${p}`).value = '';
  }
  checkEnableButtonActionSave();
}

function changePress(b, p, value) {
  if (!buttonActions[b]) return;
  buttonActions[b]['action'][p]['is-long-press'] = (value==='true');
  if (_(`mui-long-${b}-${p}`)) _(`mui-long-${b}-${p}`).style.display = value==='true' ? 'block' : 'none';
  if (_(`mui-short-${b}-${p}`)) _(`mui-short-${b}-${p}`).style.display = value==='true' ? 'none' : 'block';
  checkEnableButtonActionSave();
}

function changeCount(b, p, value) {
  if (!buttonActions[b]) return;
  buttonActions[b]['action'][p]['count'] = parseInt(value);
  if (_(`select-long-${b}-${p}`)) _(`select-long-${b}-${p}`).value = value;
  if (_(`select-short-${b}-${p}`)) _(`select-short-${b}-${p}`).value = value;
  checkEnableButtonActionSave();
}

function appendRow(b,p,v) {
  if (!_('button-actions')) return;
  const row = _('button-actions').insertRow();
  row.innerHTML = `
<td>Button ${parseInt(b)+1}</td>
<td>
  <div class="mui-select">
    <select onchange="changeAction(${b}, ${p}, parseInt(this.value));">
      <option value='0' ${v['action']===0 ? 'selected' : ''}>Unused</option>
      <option value='1' ${v['action']===1 ? 'selected' : ''}>Increase Power</option>
      <option value='2' ${v['action']===2 ? 'selected' : ''}>Go to VTX Band Menu</option>
      <option value='3' ${v['action']===3 ? 'selected' : ''}>Go to VTX Channel Menu</option>
      <option value='4' ${v['action']===4 ? 'selected' : ''}>Send VTX Settings</option>
      <option value='5' ${v['action']===5 ? 'selected' : ''}>Start WiFi</option>
      <option value='6' ${v['action']===6 ? 'selected' : ''}>Enter Binding Mode</option>
      <option value='7' ${v['action']===7 ? 'selected' : ''}>Start BLE Joystick</option>
    </select>
    <label>Action</label>
  </div>
</td>
<td>
  <div class="mui-select">
    <select id="select-press-${b}-${p}" onchange="changePress(${b}, ${p}, this.value);">
      <option value='' disabled hidden ${v['action']===0 ? 'selected' : ''}></option>
      <option value='false' ${v['is-long-press']===false ? 'selected' : ''}>Short press (click)</option>
      <option value='true' ${v['is-long-press']===true ? 'selected' : ''}>Long press (hold)</option>
    </select>
    <label>Press</label>
  </div>
</td>
<td>
  <div class="mui-select" id="mui-long-${b}-${p}" style="display:${buttonActions[b]['action'][p]['is-long-press'] ? "block": "none"};">
    <select id="select-long-${b}-${p}" onchange="changeCount(${b}, ${p}, this.value);">
      <option value='' disabled hidden ${v['action']===0 ? 'selected' : ''}></option>
      <option value='0' ${v['count']===0 ? 'selected' : ''}>for 0.5 seconds</option>
      <option value='1' ${v['count']===1 ? 'selected' : ''}>for 1 second</option>
      <option value='2' ${v['count']===2 ? 'selected' : ''}>for 1.5 seconds</option>
      <option value='3' ${v['count']===3 ? 'selected' : ''}>for 2 seconds</option>
      <option value='4' ${v['count']===4 ? 'selected' : ''}>for 2.5 seconds</option>
      <option value='5' ${v['count']===5 ? 'selected' : ''}>for 3 seconds</option>
      <option value='6' ${v['count']===6 ? 'selected' : ''}>for 3.5 seconds</option>
      <option value='7' ${v['count']===7 ? 'selected' : ''}>for 4 seconds</option>
    </select>
    <label>Count</label>
  </div>
  <div class="mui-select" id="mui-short-${b}-${p}" style="display:${buttonActions[b]['action'][p]['is-long-press'] ? "none": "block"};">
    <select id="select-short-${b}-${p}" onchange="changeCount(${b}, ${p}, this.value);">
      <option value='' disabled hidden ${v['action']===0 ? 'selected' : ''}></option>
      <option value='0' ${v['count']===0 ? 'selected' : ''}>1 time</option>
      <option value='1' ${v['count']===1 ? 'selected' : ''}>2 times</option>
      <option value='2' ${v['count']===2 ? 'selected' : ''}>3 times</option>
      <option value='3' ${v['count']===3 ? 'selected' : ''}>4 times</option>
      <option value='4' ${v['count']===4 ? 'selected' : ''}>5 times</option>
      <option value='5' ${v['count']===5 ? 'selected' : ''}>6 times</option>
      <option value='6' ${v['count']===6 ? 'selected' : ''}>7 times</option>
      <option value='7' ${v['count']===7 ? 'selected' : ''}>8 times</option>
    </select>
    <label>Count</label>
  </div>
</td>
`;
}

// ==================== Binding phrase (unchanged) ====================
md5 = function() {
  const k = [];
  let i = 0;
  for (; i < 64;) k[i] = 0 | (Math.abs(Math.sin(++i)) * 4294967296);
  function calcMD5(str) {
    let b; let c; let d; let j;
    const x = [];
    const str2 = unescape(encodeURI(str));
    let a = str2.length;
    const h = [b = 1732584193, c = -271733879, ~b, ~c];
    let i = 0;
    for (; i <= a;) x[i >> 2] |= (str2.charCodeAt(i) || 128) << 8 * (i++ % 4);
    x[str = (a + 8 >> 6) * 16 + 14] = a * 8;
    i = 0;
    for (; i < str; i += 16) {
      a = h; j = 0;
      for (; j < 64;) {
        a = [
          d = a[3],
          ((b = a[1] | 0) +
            ((d = (
              (a[0] + [
                b & (c = a[2]) | ~b & d,
                d & b | ~d & c,
                b ^ c ^ d,
                c ^ (b | ~d)
              ][a = j >> 4]
              ) +
              (k[j] + (x[[ j, 5 * j + 1, 3 * j + 5, 7 * j ][a] % 16 + i] | 0))
            )) << (a = [7,12,17,22, 5,9,14,20, 4,11,16,23, 6,10,15,21][4 * a + j++ % 4]) | d >>> 32 - a)
          ),
          b, c
        ];
      }
      for (j = 4; j;) h[--j] = h[j] + a[j];
    }
    let str = []; for (; j < 32;) str.push(((h[j >> 3] >> ((1 ^ j++ & 7) * 4)) & 15) * 16 + ((h[j >> 3] >> ((1 ^ j++ & 7) * 4)) & 15));
    return new Uint8Array(str);
  }
  return calcMD5;
}();

function isValidUidByte(s) {
  let f = parseFloat(s);
  return !isNaN(f) && isFinite(s) && Number.isInteger(f) && f >= 0 && f < 256;
}

function uidBytesFromText(text) {
  if (/^[0-9, ]+$/.test(text)) {
    let asArray = text.split(',').filter(isValidUidByte).map(Number);
    if (asArray.length >= 4 && asArray.length <= 6) {
      while (asArray.length < 6) asArray.unshift(0);
      return asArray;
    }
  }
  const bindingPhraseFull = `-DMY_BINDING_PHRASE="${text}"`;
  const bindingPhraseHashed = md5(bindingPhraseFull);
  return bindingPhraseHashed.subarray(0, 6);
}

function initBindingPhraseGen() {
  const uid = _('uid');
  if (!uid || !('addEventListener' in uid)) return;

  function setOutput(text) {
    if (text.length === 0) {
      uid.value = (originalUID || []).toString();
      updateUIDType(originalUIDType);
    } else {
      uid.value = uidBytesFromText(text.trim());
      updateUIDType('Modified');
    }
  }
  function updateValue(e) { setOutput(e.target.value); }

  if (_('phrase')) _('phrase').addEventListener('input', updateValue);
  setOutput('');
}

function updateOptions(data) {
  // --- storage radio group ---
  if (data && typeof data['storage-type'] === 'string') {
    const r = document.querySelector(`input[name="storage-type"][value="${data['storage-type']}"]`);
    if (r) r.checked = true;
  }

  // existing code follows…
  for (const [key, value] of Object.entries(data || {})) {
    if (key === 'wifi-on-interval' && value === -1) continue;
    const el = _(key);
    if (el) {
      if (el.type === 'checkbox') el.checked = value;
      else el.value = Array.isArray(value) ? value.toString() : value;
      if (el.onchange) el.onchange();
    }
  }
  if (data && data['wifi-ssid']) { if (_('homenet')) _('homenet').textContent = data['wifi-ssid']; }
  else { if (_('connect')) _('connect').style.display = 'none'; }
  if (data && data['customised'] && _('reset-options')) _('reset-options').style.display = 'block';
  if (_('submit-options')) _('submit-options').disabled = false;
}
