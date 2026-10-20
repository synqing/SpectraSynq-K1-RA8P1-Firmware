const fs=require('fs'),path=require('path'),crypto=require('crypto');
const {chromium}=require('/Users/spectrasynq/.npm/_npx/53b5ca06f5b71ebd/node_modules/playwright');
const dir=__dirname;
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'/Users/spectrasynq/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell'});
 const results=[];
 for(const id of ['song-0','song-1','clear120','sparse_reentry']){
  const page=await browser.newPage({viewport:{width:1440,height:1160}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await page.goto('file://'+dir+'/'+id+'.html');
  await page.waitForFunction(()=>window.auditionTest);
  await page.evaluate(async()=>{const a=document.querySelector('audio');a.muted=true;await a.play()});
  await page.waitForTimeout(300);
  const result=await page.evaluate(()=>{
   const a=document.querySelector('audio');a.pause();const t=window.auditionTest;let violations=0;
   for(let j=0;j<t.rows.length;j++){const us=t.rows[j].available_us;if(t.indexAt(us)!==j||t.indexAt(us-0.001)!==j-1)violations++;}
   return {duration:a.duration,playbackAdvanced:a.currentTime>0,currentTime:a.currentTime,availabilityBoundaries:t.rows.length,causalityViolations:violations,decodedPixelValues:t.pixels.length};
  });
  const captures=[];
  for(const second of (id==='sparse_reentry'?[18,23,23.01]:[18])){
   await page.evaluate(s=>document.querySelector('audio').currentTime=s,second);
   await page.waitForTimeout(100);
   const record=await page.evaluate(()=>{
    const a=document.querySelector('audio'),t=window.auditionTest,index=t.indexAt(a.currentTime*1e6),r=t.rows[index];
    return {presentation:a.currentTime,available_us:r.available_us,onset_event_id:r.onset_event_id,onset_age_ms:r.onset_age_ms,onset_strength:r.onset_strength,phase_authority:r.field.phase_authority,silence:r.field.silence,table:document.querySelector('#tracks').innerText,candidateReadout:document.querySelector('#candidateReadout').innerText};
   });
   const file=id+'-'+second+'s-srgb.png';
   await page.screenshot({path:path.join(dir,file),fullPage:true});captures.push({file,...record});
  }
  let offEqual=true;
  for(let mask=0;mask<8;mask++){
   for(const [bit,name] of [[1,'accents'],[2,'clock'],[4,'field']])await page.locator('#'+name).setChecked(Boolean(mask&bit));
   const p=await page.evaluate(()=>{
    const t=window.auditionTest,idx=t.indexAt(document.querySelector('audio').currentTime*1e6);
    const mask=Number(document.querySelector('#accents').checked)+2*Number(document.querySelector('#clock').checked)+4*Number(document.querySelector('#field').checked);
    const a=t.frame(idx,mask,0),b=t.frame(idx,mask,1);return {mask,equal:a.every((x,i)=>x===b[i])};
   });
   if((mask===0||mask===2)&&!p.equal)offEqual=false;
  }
  results.push({id,htmlSha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(dir,id+'.html'))).digest('hex'),...result,eightControlsExercised:true,disabledLayersMatchBaseline:offEqual,captures,errors});
  await page.close();
 }
 await browser.close();
 fs.writeFileSync(path.join(dir,'browser-final.json'),JSON.stringify(results,null,2));
 console.log(JSON.stringify(results,null,2));
 if(results.some(r=>!r.playbackAdvanced||r.causalityViolations||r.errors.length||!r.disabledLayersMatchBaseline))process.exitCode=1;
 const sparse=results.find(r=>r.id==='sparse_reentry');
 if(sparse.captures[0].phase_authority||sparse.captures[2].onset_strength!==1)process.exitCode=1;
})();
