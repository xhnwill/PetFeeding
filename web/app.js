export const DAY=86400;
export const service='4d500000-633a-4640-91f2-73d12144a010';
export const stateUUID='4d500001-633a-4640-91f2-73d12144a010';
export const commandUUID='4d500002-633a-4640-91f2-73d12144a010';
export function createPet(now=Date.now()/1000){
 const seed=new Uint32Array(4);crypto.getRandomValues(seed);
 return {version:'0.2.0',id:[...seed].map(x=>x.toString(16).padStart(8,'0')).join(''),species:seed[2]%3,color:seed[3]%4,stage:0,days:0,growth:0,hunger:80,mood:80,energy:80,clean:100,bond:0,poop:0,meals:0,gifts:0,sleeping:false,clockValid:true,born:Math.floor(now),last:Math.floor(now),window:0,sleepSeconds:0,stageAt:Math.floor(now)};
}
export function tick(p,now){
 now=Math.floor(now);if(now<=p.last)return p;
 while(p.last<now){
  const window=Math.floor((p.last-p.born)/DAY);
  if(window!==p.window){p.window=window;p.sleepSeconds=0;}
  const end=Math.min(now,p.born+(window+1)*DAY),dt=end-p.last;
  const bonus=p.sleeping?Math.max(0,Math.min(dt,57600-p.sleepSeconds)):0;p.sleepSeconds+=bonus;
  p.growth+=dt/(2*DAY)+bonus/(115200);
  while(p.stage<4&&p.growth+1e-8>=(p.stage+1)*30&&end-p.stageAt>=30*DAY){p.stage++;p.stageAt=end;}
  const hours=Math.floor((end-p.born)/3600)-Math.floor((p.last-p.born)/3600);
  p.hunger=Math.max(0,p.hunger-hours*(p.sleeping?1:4));
  p.energy=Math.max(0,Math.min(100,p.energy+hours*(p.sleeping?5:-2)));
  if(!p.sleeping)p.mood=Math.max(0,p.mood-hours*2);
  p.last=end;
 }p.days=Math.floor((now-p.born)/DAY);return p;
}
export function act(p,action){
 const cap=v=>Math.max(0,Math.min(100,v));
 if(action==='feed'){if(p.hunger>90)throw Error('现在已经吃饱啦');p.sleeping=false;p.hunger=cap(p.hunger+25);p.bond=cap(p.bond+1);if(++p.meals%3===0&&p.poop<3){p.poop++;p.clean=cap(p.clean-20);}}
 else if(action==='sleep')p.sleeping=true;
 else if(action==='wake')p.sleeping=false;
 else if(action==='clean'){p.poop=0;p.clean=100;}
 else if(action==='play'){if(p.energy<10)throw Error('有点累了，先睡一会吧');p.sleeping=false;p.energy-=10;p.mood=cap(p.mood+15);p.bond=cap(p.bond+2);}
 else if(action==='pet')p.mood=cap(p.mood+3);
 else throw Error('这项功能需要连接真实宠物机');
 return p;
}
if(typeof document!=='undefined')start();
function start(){
 const $=id=>document.getElementById(id);let mode='demo',device,readChr,writeChr,token='',pet,busy=false;
 try{pet=JSON.parse(localStorage.getItem('mypat-demo-v2'));if(!pet||!Number.isFinite(pet.last)||pet.version!=='0.2.0')pet=null;}catch{}
 pet??=createPet();let demo=pet;
 const feedback=s=>$('feedback').textContent=s;
 const species=['森林小兽','星云小兽','阳光小兽'],names=['芽芽','团团','小麦'];
 function draw(p,frame=Date.now()/500){
  const c=$('petCanvas').getContext('2d'),w=480,h=380;c.clearRect(0,0,w,h);
  const palettes=['#89b79e','#e8ab91','#b7a2d1','#e2bf72'],body=palettes[p.color%4];
  const ellipse=(x,y,rx,ry,col)=>{c.fillStyle=col;c.beginPath();c.ellipse(x,y,rx,ry,0,0,Math.PI*2);c.fill();};
  const bounce=p.sleeping?0:Math.sin(frame)*4,y=205+bounce,r=65+p.stage*6;
  ellipse(240,307,105,13,'#c5d3b6');
  ellipse(240-r+15,y-66,24,p.species===0?55:32,body);ellipse(240+r-15,y-66,24,p.species===0?55:32,body);
  ellipse(240,y,r,87,body);ellipse(240,y+26,42,48,'#f7e9cc');
  ellipse(240-r+7,y+69,27,14,body);ellipse(240+r-7,y+69,27,14,body);
  if(p.sleeping){c.strokeStyle='#32463b';c.lineWidth=5;for(const x of [213,267]){c.beginPath();c.arc(x,y-10,10,0,Math.PI);c.stroke();}c.font='24px serif';c.fillStyle='#6f8866';c.fillText('z Z',350,120);}
  else{ellipse(213,y-10,7,10,'#32463b');ellipse(267,y-10,7,10,'#32463b');ellipse(215,y-13,2,3,'#fff');ellipse(269,y-13,2,3,'#fff');}
  ellipse(196,y+7,10,5,'#db967d');ellipse(284,y+7,10,5,'#db967d');
  c.strokeStyle='#536651';c.lineWidth=3;c.beginPath();c.arc(240,y+7,7,0,Math.PI);c.stroke();
  c.font='25px sans-serif';for(let i=0;i<p.poop;i++)c.fillText('💩',360+i*25,300);
 }
 function render(){
  $('species').textContent=species[pet.species];$('petName').textContent='你好，我是'+names[pet.species];
  $('sleepState').textContent=pet.sleeping?'做梦中':'醒着呢';$('petAge').textContent='一起度过 '+pet.days+' 天';
  $('petStage').textContent=['幼体','成长期一','成长期二','成长期三','成熟期'][pet.stage];
  $('petLine').textContent=pet.sleeping?'嘘，我在梦里慢慢长大。':pet.hunger<30?'肚子咕噜噜，想吃点东西。':'今天也想和你待在一起。';
  $('meters').replaceChildren(...[['饱食','hunger'],['心情','mood'],['精力','energy'],['清洁','clean']].map(([name,key])=>{
   const div=document.createElement('div');div.className='meter';const label=document.createElement('span');label.textContent=name;
   const bar=document.createElement('progress');bar.max=100;bar.value=pet[key];bar.setAttribute('aria-label',name);
   const value=document.createElement('span');value.textContent=pet[key];div.append(label,bar,value);return div;
  }));
  $('clockNotice').textContent=pet.clockValid?'':'设备时间未校准：请在家长设置中校时。';
  $('growth').value=pet.stage>=4?30:pet.growth-pet.stage*30;
  $('growthDetail').textContent=pet.stage>=4?'已经长大，陪伴还会继续。':Math.min(30,pet.growth-pet.stage*30).toFixed(1)+' / 30 成长日';
  $('memories').textContent=pet.gifts?pet.gifts+' 份朋友的小礼物':'还在等第一位朋友';
  $('sleepButton').dataset.action=pet.sleeping?'wake':'sleep';$('sleepButton').querySelector('span').textContent=pet.sleeping?'轻轻叫醒':'睡个好觉';
  $('timeInfo').textContent=pet.clockValid?'时间已校准':'等待校准';draw(pet);
 }
 async function read(){
  if(mode==='ble'){const v=await readChr.readValue();return JSON.parse(new TextDecoder().decode(v));}
  if(mode==='lan'){const r=await fetch('/api/state',{headers:{'X-MyPat-Token':token},cache:'no-store'});if(!r.ok)throw Error('连接失败，请检查设备口令');return r.json();}
  tick(demo,Date.now()/1000);localStorage.setItem('mypat-demo-v2',JSON.stringify(demo));return demo;
 }
 async function send(action,extra={}){
  if(busy)throw Error('上一项操作正在完成，请稍等');busy=true;
  try{
   while(polling)await new Promise(r=>setTimeout(r,30));
   if(mode==='demo'){tick(demo,Date.now()/1000);act(demo,action);localStorage.setItem('mypat-demo-v2',JSON.stringify(demo));pet=demo;render();return;}
   const request=crypto.randomUUID(),json=JSON.stringify({action,request,...extra})+'\n';
   if(new TextEncoder().encode(json).length>510)throw Error('配置内容过长');
   if(mode==='ble'){
    const bytes=new TextEncoder().encode(json);
    for(let i=0;i<bytes.length;i+=18)await writeChr.writeValueWithResponse(bytes.slice(i,i+18));
   }else{
    const r=await fetch('/api/command',{method:'POST',headers:{'X-MyPat-Token':token,'Content-Type':'application/json'},body:json});
    if(!r.ok)throw Error('设备暂时无法接受操作');
   }
   for(let i=0;i<25;i++){await new Promise(r=>setTimeout(r,200));pet=await read();if(pet.ack===request){render();if(pet.error)throw Error(pet.error==='action_rejected'?'设备未接受此操作，请检查状态':pet.error);return;}}
   throw Error('尚未收到确认，请读取设备状态后再操作');
  }finally{busy=false;}
 }
 const run=fn=>async()=>{try{await fn();}catch(e){feedback(e.message||String(e));}};
 document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>{document.querySelectorAll('.tab').forEach(p=>p.classList.toggle('hidden',p.id!==b.dataset.tab));document.querySelectorAll('[data-tab]').forEach(p=>p.classList.toggle('active',p===b));});
 document.querySelectorAll('[data-action]').forEach(b=>b.onclick=run(async()=>{await send(b.dataset.action);feedback('小伙伴收到啦。');}));
 $('connectBle').onclick=run(async()=>{
  if(!navigator.bluetooth)throw Error('当前浏览器不支持Web Bluetooth，请尝试电脑或Android Chrome。');
  if(!window.isSecureContext)throw Error('蓝牙连接需要HTTPS或localhost安全环境。');
  device=await navigator.bluetooth.requestDevice({filters:[{services:[service]}]});
  device.addEventListener('gattserverdisconnected',()=>{if(mode==='ble'){mode='demo';pet=demo;$('connectionText').textContent='设备已断开，已返回体验模式。';feedback('蓝牙已断开，未确认的操作请在设备上检查。');render();}});
  const server=await device.gatt.connect(),s=await server.getPrimaryService(service);
  readChr=await s.getCharacteristic(stateUUID);writeChr=await s.getCharacteristic(commandUUID);
  feedback('请核对设备与系统配对数字，并在设备按OK。');
  const value=await readChr.readValue(),state=JSON.parse(new TextDecoder().decode(value));
  mode='ble';pet=state;token=state.token||'';$('connectionText').textContent='已连接真实宠物机 · '+state.id;feedback('连接成功，可以配网、校时或照顾宠物。');render();
 });
 $('connectLan').onclick=run(async()=>{
  const input=$('token').value.trim();if(!/^[a-f0-9]{16}$/.test(input))throw Error('请输入设备MENU页的16位口令');
  const previous=mode;token=input;mode='lan';try{pet=await read();}catch(e){mode=previous;throw e;}
  $('connectionText').textContent='已连接当前局域网宠物机';feedback('连接成功');render();
 });
 $('demo').onclick=()=>{mode='demo';if(device?.gatt.connected)device.gatt.disconnect();pet=demo;$('connectionText').textContent='体验模式：只保存在本浏览器。';render();};
 $('wifiForm').onsubmit=async e=>{e.preventDefault();await run(async()=>{
  if(mode!=='ble')throw Error('Wi-Fi密码请通过加密蓝牙连接配置');
  await send('wifi',{ssid:$('ssid').value,password:$('password').value});$('password').value='';feedback('配置已接受，正在连接网络。请稍后查看设备IP。');
 })();};
 $('syncTime').onclick=run(async()=>{await send('time',{epoch:Math.floor(Date.now()/1000)});feedback('校时完成。');});
 $('otaForm').onsubmit=async e=>{e.preventDefault();await run(async()=>{
  if(mode==='demo')throw Error('请先连接真实设备');const url=new URL($('otaUrl').value);
  if(url.protocol!=='https:')throw Error('仅支持HTTPS应用固件');
  await send('ota',{url:url.href});feedback('设备已开始下载，成功后重启。请保持供电。');
 })();};
 let target=0,round=0,showing=false;
 function nextRound(){
  target=crypto.getRandomValues(new Uint8Array(1))[0]%3;showing=true;
  $('gameMessage').textContent='记住这颗星星…';
  [...$('stars').children].forEach((b,i)=>b.classList.toggle('lit',i===target));
  setTimeout(()=>{if(!$('game').open)return;[...$('stars').children].forEach(b=>b.classList.remove('lit'));showing=false;$('gameMessage').textContent='刚才是哪一颗？';},1000);
 }
 for(let i=0;i<3;i++){const b=document.createElement('button');b.textContent='★';b.setAttribute('aria-label','第'+(i+1)+'颗星星');
 b.onclick=run(async()=>{if(showing)return;if(i!==target){$('gameMessage').textContent='再试试，没有关系。';return;}if(++round===3){$('game').close();await send('play');feedback('找到了所有星星，小伙伴很开心！');}else nextRound();});$('stars').append(b);}
 $('play').onclick=()=>{round=0;$('game').showModal();nextRound();};$('closeGame').onclick=()=>$('game').close();
 let polling=false;setInterval(async()=>{if(polling||busy)return;polling=true;try{pet=await read();render();}catch(e){if(mode!=='demo')feedback('读取失败：'+e.message);}finally{polling=false;}},2000);
 render();function animate(){draw(pet);requestAnimationFrame(animate);}requestAnimationFrame(animate);
}
