export const PET_ACTIONS=['idle','sleep','eat','play'];
export const FRAME_MS={idle:1000,sleep:700,eat:220,play:180};
export const ACTION_MS={idle:0,sleep:0,eat:3520,play:2880};
export function createAnimation(){return {action:'idle',started:0,duration:0};}
export function triggerAnimation(state,action,now){
 if(!PET_ACTIONS.includes(action))throw new Error('Unknown animation');
 state.action=action;state.started=now;state.duration=ACTION_MS[action];
}
export function selectAnimation(state,sleeping,playing,now){
 if(sleeping)return 'sleep';
 if(playing)return 'play';
 return now-state.started<state.duration?state.action:'idle';
}
export function animationFrame(state,action,now){
 if(action==='idle')return 0;
 const elapsed=action==='sleep'?now:Math.max(0,now-state.started);
 return Math.floor(elapsed/FRAME_MS[action])%4;
}
export function loadPetArt(){
 return ['forest','nebula','sunshine'].map(slug=>{
  const idle=new Image(),motion=new Image();
  idle.src='./assets/pets/'+slug+'.png';motion.src='./assets/pets/'+slug+'-actions.png';
  return {idle,motion};
 });
}
export function drawPetFrame(ctx,art,species,action,frame,x,y,size){
 const im=action==='idle'?art[species]?.idle:art[species]?.motion;
 if(!im?.complete||!im.naturalWidth)return false;
 ctx.imageSmoothingEnabled=false;
 if(action==='idle')ctx.drawImage(im,x,y,size,size);
 else ctx.drawImage(im,(frame%4)*96,(['sleep','eat','play'].indexOf(action))*96,96,96,x,y,size,size);
 return true;
}
