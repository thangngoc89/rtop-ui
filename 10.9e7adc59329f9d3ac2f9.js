(this.webpackJsonp=this.webpackJsonp||[]).push([[10],{252:function(e,r,t){var n=t(253);e.exports=function(e){var r=e.replace(/-/g,"+").replace(/_/g,"/");switch(r.length%4){case 0:break;case 2:r+="==";break;case 3:r+="=";break;default:throw"Illegal base64url string!"}try{return function(e){return decodeURIComponent(n(e).replace(/(.)/g,function(e,r){var t=r.charCodeAt(0).toString(16).toUpperCase();return t.length<2&&(t="0"+t),"%"+t}))}(r)}catch(e){return n(r)}}},253:function(e,r){var t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";function n(e){this.message=e}n.prototype=new Error,n.prototype.name="InvalidCharacterError",e.exports="undefined"!=typeof window&&window.atob&&window.atob.bind(window)||function(e){var r=String(e).replace(/=+$/,"");if(r.length%4==1)throw new n("'atob' failed: The string to be decoded is not correctly encoded.");for(var o,a,i=0,c=0,s="";a=r.charAt(c++);~a&&(o=i%4?64*o+a:a,i++%4)?s+=String.fromCharCode(255&o>>(-2*i&6)):0)a=t.indexOf(a);return s}},258:function(e,r,t){"use strict";var n=t(252);function o(e){this.message=e}o.prototype=new Error,o.prototype.name="InvalidTokenError",e.exports=function(e,r){if("string"!=typeof e)throw new o("Invalid token specified");var t=!0===(r=r||{}).header?0:1;try{return JSON.parse(n(e.split(".")[t]))}catch(e){throw new o("Invalid token specified: "+e.message)}},e.exports.InvalidTokenError=o}}]);