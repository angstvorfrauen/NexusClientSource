"use strict";
const avatar=document.getElementById("avatar"), avatarInput=document.getElementById("avatar-input");
let selectedAvatar=null;
function msg(id,text,error=false){const el=document.getElementById(id);el.textContent=text;el.style.color=error?"#ff8e8e":"var(--text-dim)"}
function deviceLabel(ua){
  if(/Windows/i.test(ua)) return /Windows 11/i.test(ua)?"Windows 11":"Windows";
  if(/Mac OS X/i.test(ua)) return "macOS";
  if(/Android/i.test(ua)) return "Android";
  if(/iPhone|iPad/i.test(ua)) return "iOS";
  if(/Linux/i.test(ua)) return "Linux";
  return ua||"Unknown";
}
async function load(){
 try{
  const r=await fetch("/api/settings",{credentials:"same-origin",cache:"no-store"});
  if(!r.ok) throw 0; const d=await r.json(); const u=d.user;
  document.getElementById("profile-name").textContent=u.username;
  document.getElementById("profile-role").textContent=(u.role==="admin"?"Administrator":"User account");
  document.getElementById("username").value=u.username;
  document.getElementById("email").value=u.email||"";
  document.getElementById("ip").textContent=u.lastIp||u.createdIp||"Unknown";
  document.getElementById("device").textContent=deviceLabel(u.userAgent);
  document.getElementById("created").textContent=u.createdAt?new Date(u.createdAt).toLocaleString():"Unknown";
  updateTwoFactor(u.twoFactorEnabled===true);
  const avatarWrap = avatar.parentElement;
  avatarWrap.querySelectorAll(".avatar-initial").forEach(el => el.remove());

  if(u.avatar) {
    avatar.src=u.avatar;
    avatar.alt=`${u.username || "User"} profile picture`;
    avatar.style.display="block";
    avatarWrap.classList.remove("avatar-initial-mode");
  } else {
    avatar.removeAttribute("src");
    avatar.alt="";
    avatar.style.display="none";
    avatarWrap.classList.add("avatar-initial-mode");
    const initial=document.createElement("span");
    initial.className="avatar-initial";
    initial.textContent=(u.username||"?").trim().charAt(0).toUpperCase()||"?";
    avatarWrap.prepend(initial);
  }
  if(u.role==="admin") document.getElementById("delete-account").disabled=true;
 }catch{location="/login.html"}
}
avatarInput.addEventListener("change",()=>{
 const file=avatarInput.files[0]; if(!file)return;
 if(file.size>300*1024){msg("profile-msg","Profile picture must be 300 KB or smaller.",true);avatarInput.value="";return}
 const reader=new FileReader();
 reader.onload=()=>{
   selectedAvatar=reader.result;
   const wrap=avatar.parentElement;
   wrap.querySelectorAll(".avatar-initial").forEach(el=>el.remove());
   wrap.classList.remove("avatar-initial-mode");
   avatar.src=selectedAvatar;
   avatar.alt="New profile picture";
   avatar.style.display="block";
 };
 reader.readAsDataURL(file);
});
document.getElementById("profile-form").addEventListener("submit",async e=>{
 e.preventDefault();
 const username=document.getElementById("username").value.trim(), email=document.getElementById("email").value.trim();
 try{
  const body={username,email}; if(selectedAvatar)body.avatar=selectedAvatar;
  const r=await fetch("/api/settings",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  const d=await r.json(); if(!r.ok)throw new Error(d.error||"Could not save changes.");
  msg("profile-msg","Changes saved."); selectedAvatar=null; load();
 }catch(e){msg("profile-msg",e.message,true)}
});
document.getElementById("delete-account").addEventListener("click",async()=>{
 if(!confirm("Delete your NexusClient account permanently? This cannot be undone."))return;
 try{
  const r=await fetch("/api/account/delete",{method:"POST"}), d=await r.json();
  if(!r.ok)throw new Error(d.error||"Could not delete account.");
  location="/register.html";
 }catch(e){msg("delete-msg",e.message,true)}
});
load();

function updateTwoFactor(enabled){
 const status=document.getElementById("twofa-status"),setup=document.getElementById("twofa-setup"),verify=document.getElementById("twofa-verify"),check=document.getElementById("twofa-check");
 status.textContent=enabled?"Enabled":"Disabled";
 setup.hidden=enabled;
 verify.hidden=true;
 check.hidden=!enabled;
 if(enabled) document.getElementById("twofa-code").value="";
}
document.getElementById("twofa-enable").addEventListener("click",async()=>{
 const b=document.getElementById("twofa-enable");b.disabled=true;
 try{const r=await fetch("/api/2fa/setup",{method:"POST"}),d=await r.json();if(!r.ok)throw new Error(d.error||"Could not send code.");window.__twofaPendingId=d.pendingId;document.getElementById("twofa-setup").hidden=true;document.getElementById("twofa-verify").hidden=false;document.getElementById("twofa-msg").textContent=d.message||"Code sent."}catch(e){document.getElementById("twofa-msg").textContent=e.message}finally{b.disabled=false}
});
document.getElementById("twofa-confirm").addEventListener("click",async()=>{
 const code=document.getElementById("twofa-code").value.trim(),msgEl=document.getElementById("twofa-msg");if(!/^\d{6}$/.test(code)){msgEl.textContent="Enter the 6-digit code.";return}
 const pendingId=window.__twofaPendingId;if(!pendingId){msgEl.textContent="Please request a new code.";return}
 try{const r=await fetch("/api/2fa/verify",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({pendingId,code})}),d=await r.json();if(!r.ok)throw new Error(d.error||"Verification failed.");msgEl.textContent="2FA enabled successfully.";updateTwoFactor(true)}catch(e){msgEl.textContent=e.message}
});
