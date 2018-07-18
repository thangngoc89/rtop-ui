// Generated by BUCKLESCRIPT VERSION 4.0.0, PLEASE EDIT WITH CARE

import * as Block from "bs-platform/lib/es6/block.js";
import * as Curry from "bs-platform/lib/es6/curry.js";

var error = /* array */[];


  function proxy(context, method, message) {
    return function() {
      error.push(arguments[0]);
      method.apply(context, Array.prototype.slice.apply(arguments))
    }
  }

  console.error = proxy(console, console.error, 'Error:')

;

var clearError = (
  function () {
    error = []
  }
);

function execute(code) {
  var a = evaluator.execute(code);
  if (a === "") {
    var message = error.join("\n");
    Curry._1(clearError, /* () */0);
    return /* Error */Block.__(1, [message]);
  } else {
    return /* Ok */Block.__(0, [a]);
  }
}

export {
  error ,
  clearError ,
  execute ,
  
}
/*  Not a pure module */
