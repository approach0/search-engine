/**
 * MathQuill v0.10.1               http://mathquill.com
 * by Han, Jeanine, and Mary  maintainers@mathquill.com
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL
 * was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 */

(function() {

var jQuery = window.jQuery,
  undefined,
  mqCmdId = 'mathquill-command-id',
  mqBlockId = 'mathquill-block-id',
  min = Math.min,
  max = Math.max;

function noop() {}

/**
 * A utility higher-order function that makes defining variadic
 * functions more convenient by letting you essentially define functions
 * with the last argument as a splat, i.e. the last argument "gathers up"
 * remaining arguments to the function:
 *   var doStuff = variadic(function(first, rest) { return rest; });
 *   doStuff(1, 2, 3); // => [2, 3]
 */
var __slice = [].slice;
function variadic(fn) {
  var numFixedArgs = fn.length - 1;
  return function() {
    var args = __slice.call(arguments, 0, numFixedArgs);
    var varArg = __slice.call(arguments, numFixedArgs);
    return fn.apply(this, args.concat([ varArg ]));
  };
}

/**
 * A utility higher-order function that makes combining object-oriented
 * programming and functional programming techniques more convenient:
 * given a method name and any number of arguments to be bound, returns
 * a function that calls it's first argument's method of that name (if
 * it exists) with the bound arguments and any additional arguments that
 * are passed:
 *   var sendMethod = send('method', 1, 2);
 *   var obj = { method: function() { return Array.apply(this, arguments); } };
 *   sendMethod(obj, 3, 4); // => [1, 2, 3, 4]
 *   // or more specifically,
 *   var obj2 = { method: function(one, two, three) { return one*two + three; } };
 *   sendMethod(obj2, 3); // => 5
 *   sendMethod(obj2, 4); // => 6
 */
var send = variadic(function(method, args) {
  return variadic(function(obj, moreArgs) {
    if (method in obj) return obj[method].apply(obj, args.concat(moreArgs));
  });
});

/**
 * A utility higher-order function that creates "implicit iterators"
 * from "generators": given a function that takes in a sole argument,
 * a "yield_" function, that calls "yield_" repeatedly with an object as
 * a sole argument (presumably objects being iterated over), returns
 * a function that calls it's first argument on each of those objects
 * (if the first argument is a function, it is called repeatedly with
 * each object as the first argument, otherwise it is stringified and
 * the method of that name is called on each object (if such a method
 * exists)), passing along all additional arguments:
 *   var a = [
 *     { method: function(list) { list.push(1); } },
 *     { method: function(list) { list.push(2); } },
 *     { method: function(list) { list.push(3); } }
 *   ];
 *   a.each = iterator(function(yield_) {
 *     for (var i in this) yield_(this[i]);
 *   });
 *   var list = [];
 *   a.each('method', list);
 *   list; // => [1, 2, 3]
 *   // Note that the for-in loop will yield 'each', but 'each' maps to
 *   // the function object created by iterator() which does not have a
 *   // .method() method, so that just fails silently.
 */
function iterator(generator) {
  return variadic(function(fn, args) {
    if (typeof fn !== 'function') fn = send(fn);
    var yield_ = function(obj) { return fn.apply(obj, [ obj ].concat(args)); };
    return generator.call(this, yield_);
  });
}

/**
 * sugar to make defining lots of commands easier.
 * TODO: rethink this.
 */
function bind(cons /*, args... */) {
  var args = __slice.call(arguments, 1);
  return function() {
    return cons.apply(this, args);
  };
}

/**
 * a development-only debug method.  This definition and all
 * calls to `pray` will be stripped from the minified
 * build of mathquill.
 *
 * This function must be called by name to be removed
 * at compile time.  Do not define another function
 * with the same name, and only call this function by
 * name.
 */
function pray(message, cond) {
  if (!cond) throw new Error('prayer failed: '+message);
}
var P = (function(prototype, ownProperty, undefined) {
  // helper functions that also help minification
  function isObject(o) { return typeof o === 'object'; }
  function isFunction(f) { return typeof f === 'function'; }

  // used to extend the prototypes of superclasses (which might not
  // have `.Bare`s)
  function SuperclassBare() {}

  return function P(_superclass /* = Object */, definition) {
    // handle the case where no superclass is given
    if (definition === undefined) {
      definition = _superclass;
      _superclass = Object;
    }

    // C is the class to be returned.
    //
    // It delegates to instantiating an instance of `Bare`, so that it
    // will always return a new instance regardless of the calling
    // context.
    //
    //  TODO: the Chrome inspector shows all created objects as `C`
    //        rather than `Object`.  Setting the .name property seems to
    //        have no effect.  Is there a way to override this behavior?
    function C() {
      var self = new Bare;
      if (isFunction(self.init)) self.init.apply(self, arguments);
      return self;
    }

    // C.Bare is a class with a noop constructor.  Its prototype is the
    // same as C, so that instances of C.Bare are also instances of C.
    // New objects can be allocated without initialization by calling
    // `new MyClass.Bare`.
    function Bare() {}
    C.Bare = Bare;

    // Set up the prototype of the new class.
    var _super = SuperclassBare[prototype] = _superclass[prototype];
    var proto = Bare[prototype] = C[prototype] = C.p = new SuperclassBare;

    // other variables, as a minifier optimization
    var extensions;


    // set the constructor property on the prototype, for convenience
    proto.constructor = C;

    C.mixin = function(def) {
      Bare[prototype] = C[prototype] = P(C, def)[prototype];
      return C;
    }

    return (C.open = function(def) {
      extensions = {};

      if (isFunction(def)) {
        // call the defining function with all the arguments you need
        // extensions captures the return value.
        extensions = def.call(C, proto, _super, C, _superclass);
      }
      else if (isObject(def)) {
        // if you passed an object instead, we'll take it
        extensions = def;
      }

      // ...and extend it
      if (isObject(extensions)) {
        for (var ext in extensions) {
          if (ownProperty.call(extensions, ext)) {
            proto[ext] = extensions[ext];
          }
        }
      }

      // if there's no init, we assume we're inheriting a non-pjs class, so
      // we default to applying the superclass's constructor.
      if (!isFunction(proto.init)) {
        proto.init = _superclass;
      }

      return C;
    })(definition);
  }

  // as a minifier optimization, we've closured in a few helper functions
  // and the string 'prototype' (C[p] is much shorter than C.prototype)
})('prototype', ({}).hasOwnProperty);
/*************************************************
 * Base classes of edit tree-related objects
 *
 * Only doing tree node manipulation via these
 * adopt/ disown methods guarantees well-formedness
 * of the tree.
 ************************************************/

// L = 'left'
// R = 'right'
//
// the contract is that they can be used as object properties
// and (-L) === R, and (-R) === L.
var L = -1;
var R = 1;

function prayDirection(dir) {
  pray('a direction was passed', dir === L || dir === R);
}

/**
 * Tiny extension of jQuery adding directionalized DOM manipulation methods.
 *
 * Funny how Pjs v3 almost just works with `jQuery.fn.init`.
 *
 * jQuery features that don't work on $:
 *   - jQuery.*, like jQuery.ajax, obviously (Pjs doesn't and shouldn't
 *                                            copy constructor properties)
 *
 *   - jQuery(function), the shortcut for `jQuery(document).ready(function)`,
 *     because `jQuery.fn.init` is idiosyncratic and Pjs doing, essentially,
 *     `jQuery.fn.init.apply(this, arguments)` isn't quite right, you need:
 *
 *       _.init = function(s, c) { jQuery.fn.init.call(this, s, c, $(document)); };
 *
 *     if you actually give a shit (really, don't bother),
 *     see https://github.com/jquery/jquery/blob/1.7.2/src/core.js#L889
 *
 *   - jQuery(selector), because jQuery translates that to
 *     `jQuery(document).find(selector)`, but Pjs doesn't (should it?) let
 *     you override the result of a constructor call
 *       + note that because of the jQuery(document) shortcut-ness, there's also
 *         the 3rd-argument-needs-to-be-`$(document)` thing above, but the fix
 *         for that (as can be seen above) is really easy. This problem requires
 *         a way more intrusive fix
 *
 * And that's it! Everything else just magically works because jQuery internally
 * uses `this.constructor()` everywhere (hence calling `$`), but never ever does
 * `this.constructor.find` or anything like that, always doing `jQuery.find`.
 */
var $ = P(jQuery, function(_) {
  _.insDirOf = function(dir, el) {
    return dir === L ?
      this.insertBefore(el.first()) : this.insertAfter(el.last());
  };
  _.insAtDirEnd = function(dir, el) {
    return dir === L ? this.prependTo(el) : this.appendTo(el);
  };
});

var Point = P(function(_) {
  _.parent = 0;
  _[L] = 0;
  _[R] = 0;

  _.init = function(parent, leftward, rightward) {
    this.parent = parent;
    this[L] = leftward;
    this[R] = rightward;
  };

  this.copy = function(pt) {
    return Point(pt.parent, pt[L], pt[R]);
  };
});

/**
 * MathQuill virtual-DOM tree-node abstract base class
 */
var Node = P(function(_) {
  _[L] = 0;
  _[R] = 0
  _.parent = 0;

  var id = 0;
  function uniqueNodeId() { return id += 1; }
  this.byId = {};

  _.init = function() {
    this.id = uniqueNodeId();
    Node.byId[this.id] = this;

    this.ends = {};
    this.ends[L] = 0;
    this.ends[R] = 0;
  };

  _.dispose = function() { delete Node.byId[this.id]; };

  _.toString = function() { return '{{ MathQuill Node #'+this.id+' }}'; };

  _.jQ = $();
  _.jQadd = function(jQ) { return this.jQ = this.jQ.add(jQ); };
  _.jQize = function(jQ) {
    // jQuery-ifies this.html() and links up the .jQ of all corresponding Nodes
    var jQ = $(jQ || this.html());

    function jQadd(el) {
      if (el.getAttribute) {
        var cmdId = el.getAttribute('mathquill-command-id');
        var blockId = el.getAttribute('mathquill-block-id');
        if (cmdId) Node.byId[cmdId].jQadd(el);
        if (blockId) Node.byId[blockId].jQadd(el);
      }
      for (el = el.firstChild; el; el = el.nextSibling) {
        jQadd(el);
      }
    }

    for (var i = 0; i < jQ.length; i += 1) jQadd(jQ[i]);
    return jQ;
  };

  _.createDir = function(dir, cursor) {
    prayDirection(dir);
    var node = this;
    node.jQize();
    node.jQ.insDirOf(dir, cursor.jQ);
    cursor[dir] = node.adopt(cursor.parent, cursor[L], cursor[R]);
    return node;
  };
  _.createLeftOf = function(el) { return this.createDir(L, el); };

  _.selectChildren = function(leftEnd, rightEnd) {
    return Selection(leftEnd, rightEnd);
  };

  _.bubble = iterator(function(yield_) {
    for (var ancestor = this; ancestor; ancestor = ancestor.parent) {
      var result = yield_(ancestor);
      if (result === false) break;
    }

    return this;
  });

  _.postOrder = iterator(function(yield_) {
    (function recurse(descendant) {
      descendant.eachChild(recurse);
      yield_(descendant);
    })(this);

    return this;
  });

  _.isEmpty = function() {
    return this.ends[L] === 0 && this.ends[R] === 0;
  };

  _.children = function() {
    return Fragment(this.ends[L], this.ends[R]);
  };

  _.eachChild = function() {
    var children = this.children();
    children.each.apply(children, arguments);
    return this;
  };

  _.foldChildren = function(fold, fn) {
    return this.children().fold(fold, fn);
  };

  _.withDirAdopt = function(dir, parent, withDir, oppDir) {
    Fragment(this, this).withDirAdopt(dir, parent, withDir, oppDir);
    return this;
  };

  _.adopt = function(parent, leftward, rightward) {
    Fragment(this, this).adopt(parent, leftward, rightward);
    return this;
  };

  _.disown = function() {
    Fragment(this, this).disown();
    return this;
  };

  _.remove = function() {
    this.jQ.remove();
    this.postOrder('dispose');
    return this.disown();
  };
});

function prayWellFormed(parent, leftward, rightward) {
  pray('a parent is always present', parent);
  pray('leftward is properly set up', (function() {
    // either it's empty and `rightward` is the left end child (possibly empty)
    if (!leftward) return parent.ends[L] === rightward;

    // or it's there and its [R] and .parent are properly set up
    return leftward[R] === rightward && leftward.parent === parent;
  })());

  pray('rightward is properly set up', (function() {
    // either it's empty and `leftward` is the right end child (possibly empty)
    if (!rightward) return parent.ends[R] === leftward;

    // or it's there and its [L] and .parent are properly set up
    return rightward[L] === leftward && rightward.parent === parent;
  })());
}


/**
 * An entity outside the virtual tree with one-way pointers (so it's only a
 * "view" of part of the tree, not an actual node/entity in the tree) that
 * delimits a doubly-linked list of sibling nodes.
 * It's like a fanfic love-child between HTML DOM DocumentFragment and the Range
 * classes: like DocumentFragment, its contents must be sibling nodes
 * (unlike Range, whose contents are arbitrary contiguous pieces of subtrees),
 * but like Range, it has only one-way pointers to its contents, its contents
 * have no reference to it and in fact may still be in the visible tree (unlike
 * DocumentFragment, whose contents must be detached from the visible tree
 * and have their 'parent' pointers set to the DocumentFragment).
 */
var Fragment = P(function(_) {
  _.init = function(withDir, oppDir, dir) {
    if (dir === undefined) dir = L;
    prayDirection(dir);

    pray('no half-empty fragments', !withDir === !oppDir);

    this.ends = {};

    if (!withDir) return;

    pray('withDir is passed to Fragment', withDir instanceof Node);
    pray('oppDir is passed to Fragment', oppDir instanceof Node);
    pray('withDir and oppDir have the same parent',
         withDir.parent === oppDir.parent);

    this.ends[dir] = withDir;
    this.ends[-dir] = oppDir;

    // To build the jquery collection for a fragment, accumulate elements
    // into an array and then call jQ.add once on the result. jQ.add sorts the
    // collection according to document order each time it is called, so
    // building a collection by folding jQ.add directly takes more than
    // quadratic time in the number of elements.
    //
    // https://github.com/jquery/jquery/blob/2.1.4/src/traversing.js#L112
    var accum = this.fold([], function (accum, el) {
      accum.push.apply(accum, el.jQ.get());
      return accum;
    });

    this.jQ = this.jQ.add(accum);
  };
  _.jQ = $();

  // like Cursor::withDirInsertAt(dir, parent, withDir, oppDir)
  _.withDirAdopt = function(dir, parent, withDir, oppDir) {
    return (dir === L ? this.adopt(parent, withDir, oppDir)
                      : this.adopt(parent, oppDir, withDir));
  };
  _.adopt = function(parent, leftward, rightward) {
    prayWellFormed(parent, leftward, rightward);

    var self = this;
    self.disowned = false;

    var leftEnd = self.ends[L];
    if (!leftEnd) return this;

    var rightEnd = self.ends[R];

    if (leftward) {
      // NB: this is handled in the ::each() block
      // leftward[R] = leftEnd
    } else {
      parent.ends[L] = leftEnd;
    }

    if (rightward) {
      rightward[L] = rightEnd;
    } else {
      parent.ends[R] = rightEnd;
    }

    self.ends[R][R] = rightward;

    self.each(function(el) {
      el[L] = leftward;
      el.parent = parent;
      if (leftward) leftward[R] = el;

      leftward = el;
    });

    return self;
  };

  _.disown = function() {
    var self = this;
    var leftEnd = self.ends[L];

    // guard for empty and already-disowned fragments
    if (!leftEnd || self.disowned) return self;

    self.disowned = true;

    var rightEnd = self.ends[R]
    var parent = leftEnd.parent;

    prayWellFormed(parent, leftEnd[L], leftEnd);
    prayWellFormed(parent, rightEnd, rightEnd[R]);

    if (leftEnd[L]) {
      leftEnd[L][R] = rightEnd[R];
    } else {
      parent.ends[L] = rightEnd[R];
    }

    if (rightEnd[R]) {
      rightEnd[R][L] = leftEnd[L];
    } else {
      parent.ends[R] = leftEnd[L];
    }

    return self;
  };

  _.remove = function() {
    this.jQ.remove();
    this.each('postOrder', 'dispose');
    return this.disown();
  };

  _.each = iterator(function(yield_) {
    var self = this;
    var el = self.ends[L];
    if (!el) return self;

    for (; el !== self.ends[R][R]; el = el[R]) {
      var result = yield_(el);
      if (result === false) break;
    }

    return self;
  });

  _.fold = function(fold, fn) {
    this.each(function(el) {
      fold = fn.call(this, fold, el);
    });

    return fold;
  };
});


/**
 * Registry of LaTeX commands and commands created when typing
 * a single character.
 *
 * (Commands are all subclasses of Node.)
 */
var LatexCmds = {}, CharCmds = {};
/********************************************
 * Cursor and Selection "singleton" classes
 *******************************************/

/* The main thing that manipulates the Math DOM. Makes sure to manipulate the
HTML DOM to match. */

/* Sort of singletons, since there should only be one per editable math
textbox, but any one HTML document can contain many such textboxes, so any one
JS environment could actually contain many instances. */

//A fake cursor in the fake textbox that the math is rendered in.
var Cursor = P(Point, function(_) {
  _.init = function(initParent, options) {
    this.parent = initParent;
    this.options = options;

    var jQ = this.jQ = this._jQ = $('<span class="mq-cursor">&#8203;</span>');
    //closured for setInterval
    this.blink = function(){ jQ.toggleClass('mq-blink'); };

    this.upDownCache = {};
  };

  _.show = function() {
    this.jQ = this._jQ.removeClass('mq-blink');
    if ('intervalId' in this) //already was shown, just restart interval
      clearInterval(this.intervalId);
    else { //was hidden and detached, insert this.jQ back into HTML DOM
      if (this[R]) {
        if (this.selection && this.selection.ends[L][L] === this[L])
          this.jQ.insertBefore(this.selection.jQ);
        else
          this.jQ.insertBefore(this[R].jQ.first());
      }
      else
        this.jQ.appendTo(this.parent.jQ);
      this.parent.focus();
    }
    this.intervalId = setInterval(this.blink, 500);
    return this;
  };
  _.hide = function() {
    if ('intervalId' in this)
      clearInterval(this.intervalId);
    delete this.intervalId;
    this.jQ.detach();
    this.jQ = $();
    return this;
  };

  _.withDirInsertAt = function(dir, parent, withDir, oppDir) {
    var oldParent = this.parent;
    this.parent = parent;
    this[dir] = withDir;
    this[-dir] = oppDir;
    // by contract, .blur() is called after all has been said and done
    // and the cursor has actually been moved
    if (oldParent !== parent && oldParent.blur) oldParent.blur();
  };
  _.insDirOf = function(dir, el) {
    prayDirection(dir);
    this.jQ.insDirOf(dir, el.jQ);
    this.withDirInsertAt(dir, el.parent, el[dir], el);
    this.parent.jQ.addClass('mq-hasCursor');
    return this;
  };
  _.insLeftOf = function(el) { return this.insDirOf(L, el); };
  _.insRightOf = function(el) { return this.insDirOf(R, el); };

  _.insAtDirEnd = function(dir, el) {
    prayDirection(dir);
    this.jQ.insAtDirEnd(dir, el.jQ);
    this.withDirInsertAt(dir, el, 0, el.ends[dir]);
    el.focus();
    return this;
  };
  _.insAtLeftEnd = function(el) { return this.insAtDirEnd(L, el); };
  _.insAtRightEnd = function(el) { return this.insAtDirEnd(R, el); };

  /**
   * jump up or down from one block Node to another:
   * - cache the current Point in the node we're jumping from
   * - check if there's a Point in it cached for the node we're jumping to
   *   + if so put the cursor there,
   *   + if not seek a position in the node that is horizontally closest to
   *     the cursor's current position
   */
  _.jumpUpDown = function(from, to) {
    var self = this;
    self.upDownCache[from.id] = Point.copy(self);
    var cached = self.upDownCache[to.id];
    if (cached) {
      cached[R] ? self.insLeftOf(cached[R]) : self.insAtRightEnd(cached.parent);
    }
    else {
      var pageX = self.offset().left;
      to.seek(pageX, self);
    }
  };
  _.offset = function() {
    //in Opera 11.62, .getBoundingClientRect() and hence jQuery::offset()
    //returns all 0's on inline elements with negative margin-right (like
    //the cursor) at the end of their parent, so temporarily remove the
    //negative margin-right when calling jQuery::offset()
    //Opera bug DSK-360043
    //http://bugs.jquery.com/ticket/11523
    //https://github.com/jquery/jquery/pull/717
    var self = this, offset = self.jQ.removeClass('mq-cursor').offset();
    self.jQ.addClass('mq-cursor');
    return offset;
  }
  _.unwrapGramp = function() {
    var gramp = this.parent.parent;
    var greatgramp = gramp.parent;
    var rightward = gramp[R];
    var cursor = this;

    var leftward = gramp[L];
    gramp.disown().eachChild(function(uncle) {
      if (uncle.isEmpty()) return;

      uncle.children()
        .adopt(greatgramp, leftward, rightward)
        .each(function(cousin) {
          cousin.jQ.insertBefore(gramp.jQ.first());
        })
      ;

      leftward = uncle.ends[R];
    });

    if (!this[R]) { //then find something to be rightward to insLeftOf
      if (this[L])
        this[R] = this[L][R];
      else {
        while (!this[R]) {
          this.parent = this.parent[R];
          if (this.parent)
            this[R] = this.parent.ends[L];
          else {
            this[R] = gramp[R];
            this.parent = greatgramp;
            break;
          }
        }
      }
    }
    if (this[R])
      this.insLeftOf(this[R]);
    else
      this.insAtRightEnd(greatgramp);

    gramp.jQ.remove();

    if (gramp[L].siblingDeleted) gramp[L].siblingDeleted(cursor.options, R);
    if (gramp[R].siblingDeleted) gramp[R].siblingDeleted(cursor.options, L);
  };
  _.startSelection = function() {
    var anticursor = this.anticursor = Point.copy(this);
    var ancestors = anticursor.ancestors = {}; // a map from each ancestor of
      // the anticursor, to its child that is also an ancestor; in other words,
      // the anticursor's ancestor chain in reverse order
    for (var ancestor = anticursor; ancestor.parent; ancestor = ancestor.parent) {
      ancestors[ancestor.parent.id] = ancestor;
    }
  };
  _.endSelection = function() {
    delete this.anticursor;
  };
  _.select = function() {
    var anticursor = this.anticursor;
    if (this[L] === anticursor[L] && this.parent === anticursor.parent) return false;

    // Find the lowest common ancestor (`lca`), and the ancestor of the cursor
    // whose parent is the LCA (which'll be an end of the selection fragment).
    for (var ancestor = this; ancestor.parent; ancestor = ancestor.parent) {
      if (ancestor.parent.id in anticursor.ancestors) {
        var lca = ancestor.parent;
        break;
      }
    }
    pray('cursor and anticursor in the same tree', lca);
    // The cursor and the anticursor should be in the same tree, because the
    // mousemove handler attached to the document, unlike the one attached to
    // the root HTML DOM element, doesn't try to get the math tree node of the
    // mousemove target, and Cursor::seek() based solely on coordinates stays
    // within the tree of `this` cursor's root.

    // The other end of the selection fragment, the ancestor of the anticursor
    // whose parent is the LCA.
    var antiAncestor = anticursor.ancestors[lca.id];

    // Now we have two either Nodes or Points, guaranteed to have a common
    // parent and guaranteed that if both are Points, they are not the same,
    // and we have to figure out which is the left end and which the right end
    // of the selection.
    var leftEnd, rightEnd, dir = R;

    // This is an extremely subtle algorithm.
    // As a special case, `ancestor` could be a Point and `antiAncestor` a Node
    // immediately to `ancestor`'s left.
    // In all other cases,
    // - both Nodes
    // - `ancestor` a Point and `antiAncestor` a Node
    // - `ancestor` a Node and `antiAncestor` a Point
    // `antiAncestor[R] === rightward[R]` for some `rightward` that is
    // `ancestor` or to its right, if and only if `antiAncestor` is to
    // the right of `ancestor`.
    if (ancestor[L] !== antiAncestor) {
      for (var rightward = ancestor; rightward; rightward = rightward[R]) {
        if (rightward[R] === antiAncestor[R]) {
          dir = L;
          leftEnd = ancestor;
          rightEnd = antiAncestor;
          break;
        }
      }
    }
    if (dir === R) {
      leftEnd = antiAncestor;
      rightEnd = ancestor;
    }

    // only want to select Nodes up to Points, can't select Points themselves
    if (leftEnd instanceof Point) leftEnd = leftEnd[R];
    if (rightEnd instanceof Point) rightEnd = rightEnd[L];

    this.hide().selection = lca.selectChildren(leftEnd, rightEnd);
    this.insDirOf(dir, this.selection.ends[dir]);
    this.selectionChanged();
    return true;
  };

  _.clearSelection = function() {
    if (this.selection) {
      this.selection.clear();
      delete this.selection;
      this.selectionChanged();
    }
    return this;
  };
  _.deleteSelection = function() {
    if (!this.selection) return;

    this[L] = this.selection.ends[L][L];
    this[R] = this.selection.ends[R][R];
    this.selection.remove();
    this.selectionChanged();
    delete this.selection;
  };
  _.replaceSelection = function() {
    var seln = this.selection;
    if (seln) {
      this[L] = seln.ends[L][L];
      this[R] = seln.ends[R][R];
      delete this.selection;
    }
    return seln;
  };
});

var Selection = P(Fragment, function(_, super_) {
  _.init = function() {
    super_.init.apply(this, arguments);
    this.jQ = this.jQ.wrapAll('<span class="mq-selection"></span>').parent();
      //can't do wrapAll(this.jQ = $(...)) because wrapAll will clone it
  };
  _.adopt = function() {
    this.jQ.replaceWith(this.jQ = this.jQ.children());
    return super_.adopt.apply(this, arguments);
  };
  _.clear = function() {
    // using the browser's native .childNodes property so that we
    // don't discard text nodes.
    this.jQ.replaceWith(this.jQ[0].childNodes);
    return this;
  };
  _.join = function(methodName) {
    return this.fold('', function(fold, child) {
      return fold + child[methodName]();
    });
  };
});
/*********************************************
 * Controller for a MathQuill instance,
 * on which services are registered with
 *
 *   Controller.open(function(_) { ... });
 *
 ********************************************/

var Controller = P(function(_) {
  _.init = function(root, container, options) {
    this.id = root.id;
    this.data = {};

    this.root = root;
    this.container = container;
    this.options = options;

    root.controller = this;

    this.cursor = root.cursor = Cursor(root, options);
    // TODO: stop depending on root.cursor, and rm it
  };

  _.handle = function(name, dir) {
    var handlers = this.options.handlers;
    if (handlers && handlers.fns[name]) {
      var mq = handlers.APIClasses[this.KIND_OF_MQ](this);
      if (dir === L || dir === R) handlers.fns[name](dir, mq);
      else handlers.fns[name](mq);
    }
  };

  var notifyees = [];
  this.onNotify = function(f) { notifyees.push(f); };
  _.notify = function() {
    for (var i = 0; i < notifyees.length; i += 1) {
      notifyees[i].apply(this.cursor, arguments);
    }
    return this;
  };
});
/*********************************************************
 * The publicly exposed MathQuill API.
 ********************************************************/

var API = {}, Options = P(), optionProcessors = {}, Progenote = P(), EMBEDS = {};

/**
 * Interface Versioning (#459, #495) to allow us to virtually guarantee
 * backcompat. v0.10.x introduces it, so for now, don't completely break the
 * API for people who don't know about it, just complain with console.warn().
 *
 * The methods are shimmed in outro.js so that MQ.MathField.prototype etc can
 * be accessed.
 */
function insistOnInterVer() {
  if (window.console) console.warn(
    'You are using the MathQuill API without specifying an interface version, ' +
    'which will fail in v1.0.0. You can fix this easily by doing this before ' +
    'doing anything else:\n' +
    '\n' +
    '    MathQuill = MathQuill.getInterface(1);\n' +
    '    // now MathQuill.MathField() works like it used to\n' +
    '\n' +
    'See also the "`dev` branch (2014–2015) → v0.10.0 Migration Guide" at\n' +
    '  https://github.com/mathquill/mathquill/wiki/%60dev%60-branch-(2014%E2%80%932015)-%E2%86%92-v0.10.0-Migration-Guide'
  );
}
// globally exported API object
function MathQuill(el) {
  insistOnInterVer();
  return MQ1(el);
};
MathQuill.prototype = Progenote.p;
MathQuill.interfaceVersion = function(v) {
  // shim for #459-era interface versioning (ended with #495)
  if (v !== 1) throw 'Only interface version 1 supported. You specified: ' + v;
  insistOnInterVer = function() {
    if (window.console) console.warn(
      'You called MathQuill.interfaceVersion(1); to specify the interface ' +
      'version, which will fail in v1.0.0. You can fix this easily by doing ' +
      'this before doing anything else:\n' +
      '\n' +
      '    MathQuill = MathQuill.getInterface(1);\n' +
      '    // now MathQuill.MathField() works like it used to\n' +
      '\n' +
      'See also the "`dev` branch (2014–2015) → v0.10.0 Migration Guide" at\n' +
      '  https://github.com/mathquill/mathquill/wiki/%60dev%60-branch-(2014%E2%80%932015)-%E2%86%92-v0.10.0-Migration-Guide'
    );
  };
  insistOnInterVer();
  return MathQuill;
};
MathQuill.getInterface = getInterface;

var MIN = getInterface.MIN = 1, MAX = getInterface.MAX = 2;
function getInterface(v) {
  if (!(MIN <= v && v <= MAX)) throw 'Only interface versions between ' +
    MIN + ' and ' + MAX + ' supported. You specified: ' + v;

  /**
   * Function that takes an HTML element and, if it's the root HTML element of a
   * static math or math or text field, returns an API object for it (else, null).
   *
   *   var mathfield = MQ.MathField(mathFieldSpan);
   *   assert(MQ(mathFieldSpan).id === mathfield.id);
   *   assert(MQ(mathFieldSpan).id === MQ(mathFieldSpan).id);
   *
   */
  function MQ(el) {
    if (!el || !el.nodeType) return null; // check that `el` is a HTML element, using the
      // same technique as jQuery: https://github.com/jquery/jquery/blob/679536ee4b7a92ae64a5f58d90e9cc38c001e807/src/core/init.js#L92
    var blockId = $(el).children('.mq-root-block').attr(mqBlockId);
    var ctrlr = blockId && Node.byId[blockId].controller;
    return ctrlr ? APIClasses[ctrlr.KIND_OF_MQ](ctrlr) : null;
  };
  var APIClasses = {};

  MQ.L = L;
  MQ.R = R;

  function config(currentOptions, newOptions) {
    if (newOptions && newOptions.handlers) {
      newOptions.handlers = { fns: newOptions.handlers, APIClasses: APIClasses };
    }
    for (var name in newOptions) if (newOptions.hasOwnProperty(name)) {
      var value = newOptions[name], processor = optionProcessors[name];
      currentOptions[name] = (processor ? processor(value) : value);
    }
  }
  MQ.config = function(opts) { config(Options.p, opts); return this; };
  MQ.registerEmbed = function(name, options) {
    if (!/^[a-z][a-z0-9]*$/i.test(name)) {
      throw 'Embed name must start with letter and be only letters and digits';
    }
    EMBEDS[name] = options;
  };

  var AbstractMathQuill = APIClasses.AbstractMathQuill = P(Progenote, function(_) {
    _.init = function(ctrlr) {
      this.__controller = ctrlr;
      this.__options = ctrlr.options;
      this.id = ctrlr.id;
      this.data = ctrlr.data;
    };
    _.__mathquillify = function(classNames) {
      var ctrlr = this.__controller, root = ctrlr.root, el = ctrlr.container;
      ctrlr.createTextarea();

      var contents = el.addClass(classNames).contents().detach();
      root.jQ =
        $('<span class="mq-root-block"/>').attr(mqBlockId, root.id).appendTo(el);
      this.latex(contents.text());

      this.revert = function() {
        return el.empty().unbind('.mathquill')
        .removeClass('mq-editable-field mq-math-mode mq-text-mode')
        .append(contents);
      };
    };
    _.config = function(opts) { config(this.__options, opts); return this; };
    _.el = function() { return this.__controller.container[0]; };
    _.text = function() { return this.__controller.exportText(); };
    _.latex = function(latex) {
      if (arguments.length > 0) {
        this.__controller.renderLatexMath(latex);
        if (this.__controller.blurred) this.__controller.cursor.hide().parent.blur();
        return this;
      }
      return this.__controller.exportLatex();
    };
    _.html = function() {
      return this.__controller.root.jQ.html()
        .replace(/ mathquill-(?:command|block)-id="?\d+"?/g, '')
        .replace(/<span class="?mq-cursor( mq-blink)?"?>.?<\/span>/i, '')
        .replace(/ mq-hasCursor|mq-hasCursor ?/, '')
        .replace(/ class=(""|(?= |>))/g, '');
    };
    _.reflow = function() {
      this.__controller.root.postOrder('reflow');
      return this;
    };
  });
  MQ.prototype = AbstractMathQuill.prototype;

  APIClasses.EditableField = P(AbstractMathQuill, function(_, super_) {
    _.__mathquillify = function() {
      super_.__mathquillify.apply(this, arguments);
      this.__controller.editable = true;
      this.__controller.delegateMouseEvents();
      this.__controller.editablesTextareaEvents();
      return this;
    };
    _.focus = function() { this.__controller.textarea.focus(); return this; };
    _.blur = function() { this.__controller.textarea.blur(); return this; };
    _.write = function(latex) {
      this.__controller.writeLatex(latex);
      this.__controller.scrollHoriz();
      if (this.__controller.blurred) this.__controller.cursor.hide().parent.blur();
      return this;
    };
    _.cmd = function(cmd) {
      var ctrlr = this.__controller.notify(), cursor = ctrlr.cursor;
      if (/^\\[a-z]+$/i.test(cmd)) {
        cmd = cmd.slice(1);
        var klass = LatexCmds[cmd];
        if (klass) {
          cmd = klass(cmd);
          if (cursor.selection) cmd.replaces(cursor.replaceSelection());
          cmd.createLeftOf(cursor.show());
          this.__controller.scrollHoriz();
        }
        else /* TODO: API needs better error reporting */;
      }
      else cursor.parent.write(cursor, cmd);
      if (ctrlr.blurred) cursor.hide().parent.blur();
      return this;
    };
    _.select = function() {
      var ctrlr = this.__controller;
      ctrlr.notify('move').cursor.insAtRightEnd(ctrlr.root);
      while (ctrlr.cursor[L]) ctrlr.selectLeft();
      return this;
    };
    _.clearSelection = function() {
      this.__controller.cursor.clearSelection();
      return this;
    };

    _.moveToDirEnd = function(dir) {
      this.__controller.notify('move').cursor.insAtDirEnd(dir, this.__controller.root);
      return this;
    };
    _.moveToLeftEnd = function() { return this.moveToDirEnd(L); };
    _.moveToRightEnd = function() { return this.moveToDirEnd(R); };

    _.keystroke = function(keys) {
      var keys = keys.replace(/^\s+|\s+$/g, '').split(/\s+/);
      for (var i = 0; i < keys.length; i += 1) {
        this.__controller.keystroke(keys[i], { preventDefault: noop });
      }
      return this;
    };
    _.typedText = function(text) {
      for (var i = 0; i < text.length; i += 1) this.__controller.typedText(text.charAt(i));
      return this;
    };
    _.dropEmbedded = function(pageX, pageY, options) {
      var clientX = pageX - $(window).scrollLeft();
      var clientY = pageY - $(window).scrollTop();

      var el = document.elementFromPoint(clientX, clientY);
      this.__controller.seek($(el), pageX, pageY);
      var cmd = Embed().setOptions(options);
      cmd.createLeftOf(this.__controller.cursor);
    };
  });
  MQ.EditableField = function() { throw "wtf don't call me, I'm 'abstract'"; };
  MQ.EditableField.prototype = APIClasses.EditableField.prototype;

  /**
   * Export the API functions that MathQuill-ify an HTML element into API objects
   * of each class. If the element had already been MathQuill-ified but into a
   * different kind (or it's not an HTML element), return null.
   */
  for (var kind in API) (function(kind, defAPIClass) {
    var APIClass = APIClasses[kind] = defAPIClass(APIClasses);
    MQ[kind] = function(el, opts) {
      var mq = MQ(el);
      if (mq instanceof APIClass || !el || !el.nodeType) return mq;
      var ctrlr = Controller(APIClass.RootBlock(), $(el), Options());
      ctrlr.KIND_OF_MQ = kind;
      return APIClass(ctrlr).__mathquillify(opts, v);
    };
    MQ[kind].prototype = APIClass.prototype;
  }(kind, API[kind]));

  return MQ;
}

MathQuill.noConflict = function() {
  window.MathQuill = origMathQuill;
  return MathQuill;
};
var origMathQuill = window.MathQuill;
window.MathQuill = MathQuill;

function RootBlockMixin(_) {
  var names = 'moveOutOf deleteOutOf selectOutOf upOutOf downOutOf'.split(' ');
  for (var i = 0; i < names.length; i += 1) (function(name) {
    _[name] = function(dir) { this.controller.handle(name, dir); };
  }(names[i]));
  _.reflow = function() {
    this.controller.handle('reflow');
    this.controller.handle('edited');
    this.controller.handle('edit');
  };
}
var Parser = P(function(_, super_, Parser) {
  // The Parser object is a wrapper for a parser function.
  // Externally, you use one to parse a string by calling
  //   var result = SomeParser.parse('Me Me Me! Parse Me!');
  // You should never call the constructor, rather you should
  // construct your Parser from the base parsers and the
  // parser combinator methods.

  function parseError(stream, message) {
    if (stream) {
      stream = "'"+stream+"'";
    }
    else {
      stream = 'EOF';
    }

    throw 'Parse Error: '+message+' at '+stream;
  }

  _.init = function(body) { this._ = body; };

  _.parse = function(stream) {
    return this.skip(eof)._(''+stream, success, parseError);

    function success(stream, result) { return result; }
  };

  // -*- primitive combinators -*- //
  _.or = function(alternative) {
    pray('or is passed a parser', alternative instanceof Parser);

    var self = this;

    return Parser(function(stream, onSuccess, onFailure) {
      return self._(stream, onSuccess, failure);

      function failure(newStream) {
        return alternative._(stream, onSuccess, onFailure);
      }
    });
  };

  _.then = function(next) {
    var self = this;

    return Parser(function(stream, onSuccess, onFailure) {
      return self._(stream, success, onFailure);

      function success(newStream, result) {
        var nextParser = (next instanceof Parser ? next : next(result));
        pray('a parser is returned', nextParser instanceof Parser);
        return nextParser._(newStream, onSuccess, onFailure);
      }
    });
  };

  // -*- optimized iterative combinators -*- //
  _.many = function() {
    var self = this;

    return Parser(function(stream, onSuccess, onFailure) {
      var xs = [];
      while (self._(stream, success, failure));
      return onSuccess(stream, xs);

      function success(newStream, x) {
        stream = newStream;
        xs.push(x);
        return true;
      }

      function failure() {
        return false;
      }
    });
  };

  _.times = function(min, max) {
    if (arguments.length < 2) max = min;
    var self = this;

    return Parser(function(stream, onSuccess, onFailure) {
      var xs = [];
      var result = true;
      var failure;

      for (var i = 0; i < min; i += 1) {
        result = self._(stream, success, firstFailure);
        if (!result) return onFailure(stream, failure);
      }

      for (; i < max && result; i += 1) {
        result = self._(stream, success, secondFailure);
      }

      return onSuccess(stream, xs);

      function success(newStream, x) {
        xs.push(x);
        stream = newStream;
        return true;
      }

      function firstFailure(newStream, msg) {
        failure = msg;
        stream = newStream;
        return false;
      }

      function secondFailure(newStream, msg) {
        return false;
      }
    });
  };

  // -*- higher-level combinators -*- //
  _.result = function(res) { return this.then(succeed(res)); };
  _.atMost = function(n) { return this.times(0, n); };
  _.atLeast = function(n) {
    var self = this;
    return self.times(n).then(function(start) {
      return self.many().map(function(end) {
        return start.concat(end);
      });
    });
  };

  _.map = function(fn) {
    return this.then(function(result) { return succeed(fn(result)); });
  };

  _.skip = function(two) {
    return this.then(function(result) { return two.result(result); });
  };

  // -*- primitive parsers -*- //
  var string = this.string = function(str) {
    var len = str.length;
    var expected = "expected '"+str+"'";

    return Parser(function(stream, onSuccess, onFailure) {
      var head = stream.slice(0, len);

      if (head === str) {
        return onSuccess(stream.slice(len), head);
      }
      else {
        return onFailure(stream, expected);
      }
    });
  };

  var regex = this.regex = function(re) {
    pray('regexp parser is anchored', re.toString().charAt(1) === '^');

    var expected = 'expected '+re;

    return Parser(function(stream, onSuccess, onFailure) {
      var match = re.exec(stream);

      if (match) {
        var result = match[0];
        return onSuccess(stream.slice(result.length), result);
      }
      else {
        return onFailure(stream, expected);
      }
    });
  };

  var succeed = Parser.succeed = function(result) {
    return Parser(function(stream, onSuccess) {
      return onSuccess(stream, result);
    });
  };

  var fail = Parser.fail = function(msg) {
    return Parser(function(stream, _, onFailure) {
      return onFailure(stream, msg);
    });
  };

  var letter = Parser.letter = regex(/^[a-z]/i);
  var letters = Parser.letters = regex(/^[a-z]*/i);
  var digit = Parser.digit = regex(/^[0-9]/);
  var digits = Parser.digits = regex(/^[0-9]*/);
  var whitespace = Parser.whitespace = regex(/^\s+/);
  var optWhitespace = Parser.optWhitespace = regex(/^\s*/);

  var any = Parser.any = Parser(function(stream, onSuccess, onFailure) {
    if (!stream) return onFailure(stream, 'expected any character');

    return onSuccess(stream.slice(1), stream.charAt(0));
  });

  var all = Parser.all = Parser(function(stream, onSuccess, onFailure) {
    return onSuccess('', stream);
  });

  var eof = Parser.eof = Parser(function(stream, onSuccess, onFailure) {
    if (stream) return onFailure(stream, 'expected EOF');

    return onSuccess(stream, stream);
  });
});
/*************************************************
 * Sane Keyboard Events Shim
 *
 * An abstraction layer wrapping the textarea in
 * an object with methods to manipulate and listen
 * to events on, that hides all the nasty cross-
 * browser incompatibilities behind a uniform API.
 *
 * Design goal: This is a *HARD* internal
 * abstraction barrier. Cross-browser
 * inconsistencies are not allowed to leak through
 * and be dealt with by event handlers. All future
 * cross-browser issues that arise must be dealt
 * with here, and if necessary, the API updated.
 *
 * Organization:
 * - key values map and stringify()
 * - saneKeyboardEvents()
 *    + defer() and flush()
 *    + event handler logic
 *    + attach event handlers and export methods
 ************************************************/

var saneKeyboardEvents = (function() {
  // The following [key values][1] map was compiled from the
  // [DOM3 Events appendix section on key codes][2] and
  // [a widely cited report on cross-browser tests of key codes][3],
  // except for 10: 'Enter', which I've empirically observed in Safari on iOS
  // and doesn't appear to conflict with any other known key codes.
  //
  // [1]: http://www.w3.org/TR/2012/WD-DOM-Level-3-Events-20120614/#keys-keyvalues
  // [2]: http://www.w3.org/TR/2012/WD-DOM-Level-3-Events-20120614/#fixed-virtual-key-codes
  // [3]: http://unixpapa.com/js/key.html
  var KEY_VALUES = {
    8: 'Backspace',
    9: 'Tab',

    10: 'Enter', // for Safari on iOS

    13: 'Enter',

    16: 'Shift',
    17: 'Control',
    18: 'Alt',
    20: 'CapsLock',

    27: 'Esc',

    32: 'Spacebar',

    33: 'PageUp',
    34: 'PageDown',
    35: 'End',
    36: 'Home',

    37: 'Left',
    38: 'Up',
    39: 'Right',
    40: 'Down',

    45: 'Insert',

    46: 'Del',

    144: 'NumLock'
  };

  // To the extent possible, create a normalized string representation
  // of the key combo (i.e., key code and modifier keys).
  function stringify(evt) {
    var which = evt.which || evt.keyCode;
    var keyVal = KEY_VALUES[which];
    var key;
    var modifiers = [];

    if (evt.ctrlKey) modifiers.push('Ctrl');
    if (evt.originalEvent && evt.originalEvent.metaKey) modifiers.push('Meta');
    if (evt.altKey) modifiers.push('Alt');
    if (evt.shiftKey) modifiers.push('Shift');

    key = keyVal || String.fromCharCode(which);

    if (!modifiers.length && !keyVal) return key;

    modifiers.push(key);
    return modifiers.join('-');
  }

  // create a keyboard events shim that calls callbacks at useful times
  // and exports useful public methods
  return function saneKeyboardEvents(el, handlers) {
    var keydown = null;
    var keypress = null;

    var textarea = jQuery(el);
    var target = jQuery(handlers.container || textarea);

    // checkTextareaFor() is called after keypress or paste events to
    // say "Hey, I think something was just typed" or "pasted" (resp.),
    // so that at all subsequent opportune times (next event or timeout),
    // will check for expected typed or pasted text.
    // Need to check repeatedly because #135: in Safari 5.1 (at least),
    // after selecting something and then typing, the textarea is
    // incorrectly reported as selected during the input event (but not
    // subsequently).
    var checkTextarea = noop, timeoutId;
    function checkTextareaFor(checker) {
      checkTextarea = checker;
      clearTimeout(timeoutId);
      timeoutId = setTimeout(checker);
    }
    target.bind('keydown keypress input keyup focusout paste', function(e) { checkTextarea(e); });


    // -*- public methods -*- //
    function select(text) {
      // check textarea at least once/one last time before munging (so
      // no race condition if selection happens after keypress/paste but
      // before checkTextarea), then never again ('cos it's been munged)
      checkTextarea();
      checkTextarea = noop;
      clearTimeout(timeoutId);

      textarea.val(text);
      if (text && textarea[0].select) textarea[0].select();
      shouldBeSelected = !!text;
    }
    var shouldBeSelected = false;

    // -*- helper subroutines -*- //

    // Determine whether there's a selection in the textarea.
    // This will always return false in IE < 9, which don't support
    // HTMLTextareaElement::selection{Start,End}.
    function hasSelection() {
      var dom = textarea[0];

      if (!('selectionStart' in dom)) return false;
      return dom.selectionStart !== dom.selectionEnd;
    }

    function handleKey() {
      handlers.keystroke(stringify(keydown), keydown);
    }

    // -*- event handlers -*- //
    function onKeydown(e) {
      keydown = e;
      keypress = null;

      if (shouldBeSelected) checkTextareaFor(function(e) {
        if (!(e && e.type === 'focusout') && textarea[0].select) {
          textarea[0].select(); // re-select textarea in case it's an unrecognized
        }
        checkTextarea = noop; // key that clears the selection, then never
        clearTimeout(timeoutId); // again, 'cos next thing might be blur
      });

      handleKey();
    }

    function onKeypress(e) {
      // call the key handler for repeated keypresses.
      // This excludes keypresses that happen directly
      // after keydown.  In that case, there will be
      // no previous keypress, so we skip it here
      if (keydown && keypress) handleKey();

      keypress = e;

      checkTextareaFor(typedText);
    }
    function typedText() {
      // If there is a selection, the contents of the textarea couldn't
      // possibly have just been typed in.
      // This happens in browsers like Firefox and Opera that fire
      // keypress for keystrokes that are not text entry and leave the
      // selection in the textarea alone, such as Ctrl-C.
      // Note: we assume that browsers that don't support hasSelection()
      // also never fire keypress on keystrokes that are not text entry.
      // This seems reasonably safe because:
      // - all modern browsers including IE 9+ support hasSelection(),
      //   making it extremely unlikely any browser besides IE < 9 won't
      // - as far as we know IE < 9 never fires keypress on keystrokes
      //   that aren't text entry, which is only as reliable as our
      //   tests are comprehensive, but the IE < 9 way to do
      //   hasSelection() is poorly documented and is also only as
      //   reliable as our tests are comprehensive
      // If anything like #40 or #71 is reported in IE < 9, see
      // b1318e5349160b665003e36d4eedd64101ceacd8
      if (hasSelection()) return;

      var text = textarea.val();
      if (text.length === 1) {
        textarea.val('');
        handlers.typedText(text);
      } // in Firefox, keys that don't type text, just clear seln, fire keypress
      // https://github.com/mathquill/mathquill/issues/293#issuecomment-40997668
      else if (text && textarea[0].select) textarea[0].select(); // re-select if that's why we're here
    }

    function onBlur() { keydown = keypress = null; }

    function onPaste(e) {
      // browsers are dumb.
      //
      // In Linux, middle-click pasting causes onPaste to be called,
      // when the textarea is not necessarily focused.  We focus it
      // here to ensure that the pasted text actually ends up in the
      // textarea.
      //
      // It's pretty nifty that by changing focus in this handler,
      // we can change the target of the default action.  (This works
      // on keydown too, FWIW).
      //
      // And by nifty, we mean dumb (but useful sometimes).
      textarea.focus();

      checkTextareaFor(pastedText);
    }
    function pastedText() {
      var text = textarea.val();
      textarea.val('');
      if (text) handlers.paste(text);
    }

    // -*- attach event handlers -*- //
    target.bind({
      keydown: onKeydown,
      keypress: onKeypress,
      focusout: onBlur,
      paste: onPaste
    });

    // -*- export public methods -*- //
    return {
      select: select
    };
  };
}());
/***********************************************
 * Export math in a human-readable text format
 * As you can see, only half-baked so far.
 **********************************************/

Controller.open(function(_, super_) {
  _.exportText = function() {
    return this.root.foldChildren('', function(text, child) {
      return text + child.text();
    });
  };
});
Controller.open(function(_) {
  _.focusBlurEvents = function() {
    var ctrlr = this, root = ctrlr.root, cursor = ctrlr.cursor;
    var blurTimeout;
    ctrlr.textarea.focus(function() {
      ctrlr.blurred = false;
      clearTimeout(blurTimeout);
      ctrlr.container.addClass('mq-focused');
      if (!cursor.parent)
        cursor.insAtRightEnd(root);
      if (cursor.selection) {
        cursor.selection.jQ.removeClass('mq-blur');
        ctrlr.selectionChanged(); //re-select textarea contents after tabbing away and back
      }
      else
        cursor.show();
    }).blur(function() {
      ctrlr.blurred = true;
      blurTimeout = setTimeout(function() { // wait for blur on window; if
        root.postOrder('intentionalBlur'); // none, intentional blur: #264
        cursor.clearSelection().endSelection();
        blur();
      });
      $(window).on('blur', windowBlur);
    });
    function windowBlur() { // blur event also fired on window, just switching
      clearTimeout(blurTimeout); // tabs/windows, not intentional blur
      if (cursor.selection) cursor.selection.jQ.addClass('mq-blur');
      blur();
    }
    function blur() { // not directly in the textarea blur handler so as to be
      cursor.hide().parent.blur(); // synchronous with/in the same frame as
      ctrlr.container.removeClass('mq-focused'); // clearing/blurring selection
      $(window).off('blur', windowBlur);
    }
    ctrlr.blurred = true;
    cursor.hide().parent.blur();
  };
});

/**
 * TODO: I wanted to move MathBlock::focus and blur here, it would clean
 * up lots of stuff like, TextBlock::focus is set to MathBlock::focus
 * and TextBlock::blur calls MathBlock::blur, when instead they could
 * use inheritance and super_.
 *
 * Problem is, there's lots of calls to .focus()/.blur() on nodes
 * outside Controller::focusBlurEvents(), such as .postOrder('blur') on
 * insertion, which if MathBlock::blur becomes Node::blur, would add the
 * 'blur' CSS class to all Symbol's (because .isEmpty() is true for all
 * of them).
 *
 * I'm not even sure there aren't other troublesome calls to .focus() or
 * .blur(), so this is TODO for now.
 */
/*****************************************
 * Deals with the browser DOM events from
 * interaction with the typist.
 ****************************************/

Controller.open(function(_) {
  _.keystroke = function(key, evt) {
    this.cursor.parent.keystroke(key, evt, this);
  };
});

Node.open(function(_) {
  _.keystroke = function(key, e, ctrlr) {
    var cursor = ctrlr.cursor;

    switch (key) {
    case 'Ctrl-Shift-Backspace':
    case 'Ctrl-Backspace':
      ctrlr.ctrlDeleteDir(L);
      break;

    case 'Shift-Backspace':
    case 'Backspace':
      ctrlr.backspace();
      break;

    // Tab or Esc -> go one block right if it exists, else escape right.
    case 'Esc':
    case 'Tab':
      ctrlr.escapeDir(R, key, e);
      return;

    // Shift-Tab -> go one block left if it exists, else escape left.
    case 'Shift-Tab':
    case 'Shift-Esc':
      ctrlr.escapeDir(L, key, e);
      return;

    // End -> move to the end of the current block.
    case 'End':
      ctrlr.notify('move').cursor.insAtRightEnd(cursor.parent);
      break;

    // Ctrl-End -> move all the way to the end of the root block.
    case 'Ctrl-End':
      ctrlr.notify('move').cursor.insAtRightEnd(ctrlr.root);
      break;

    // Shift-End -> select to the end of the current block.
    case 'Shift-End':
      while (cursor[R]) {
        ctrlr.selectRight();
      }
      break;

    // Ctrl-Shift-End -> select to the end of the root block.
    case 'Ctrl-Shift-End':
      while (cursor[R] || cursor.parent !== ctrlr.root) {
        ctrlr.selectRight();
      }
      break;

    // Home -> move to the start of the root block or the current block.
    case 'Home':
      ctrlr.notify('move').cursor.insAtLeftEnd(cursor.parent);
      break;

    // Ctrl-Home -> move to the start of the current block.
    case 'Ctrl-Home':
      ctrlr.notify('move').cursor.insAtLeftEnd(ctrlr.root);
      break;

    // Shift-Home -> select to the start of the current block.
    case 'Shift-Home':
      while (cursor[L]) {
        ctrlr.selectLeft();
      }
      break;

    // Ctrl-Shift-Home -> move to the start of the root block.
    case 'Ctrl-Shift-Home':
      while (cursor[L] || cursor.parent !== ctrlr.root) {
        ctrlr.selectLeft();
      }
      break;

    case 'Left': ctrlr.moveLeft(); break;
    case 'Shift-Left': ctrlr.selectLeft(); break;
    case 'Ctrl-Left': break;

    case 'Right': ctrlr.moveRight(); break;
    case 'Shift-Right': ctrlr.selectRight(); break;
    case 'Ctrl-Right': break;

    case 'Up': ctrlr.moveUp(); break;
    case 'Down': ctrlr.moveDown(); break;

    case 'Shift-Up':
      if (cursor[L]) {
        while (cursor[L]) ctrlr.selectLeft();
      } else {
        ctrlr.selectLeft();
      }

    case 'Shift-Down':
      if (cursor[R]) {
        while (cursor[R]) ctrlr.selectRight();
      }
      else {
        ctrlr.selectRight();
      }

    case 'Ctrl-Up': break;
    case 'Ctrl-Down': break;

    case 'Ctrl-Shift-Del':
    case 'Ctrl-Del':
      ctrlr.ctrlDeleteDir(R);
      break;

    case 'Shift-Del':
    case 'Del':
      ctrlr.deleteForward();
      break;

    case 'Meta-A':
    case 'Ctrl-A':
      ctrlr.notify('move').cursor.insAtRightEnd(ctrlr.root);
      while (cursor[L]) ctrlr.selectLeft();
      break;

    default:
      return;
    }
    e.preventDefault();
    ctrlr.scrollHoriz();
  };

  _.moveOutOf = // called by Controller::escapeDir, moveDir
  _.moveTowards = // called by Controller::moveDir
  _.deleteOutOf = // called by Controller::deleteDir
  _.deleteTowards = // called by Controller::deleteDir
  _.unselectInto = // called by Controller::selectDir
  _.selectOutOf = // called by Controller::selectDir
  _.selectTowards = // called by Controller::selectDir
    function() { pray('overridden or never called on this node'); };
});

Controller.open(function(_) {
  this.onNotify(function(e) {
    if (e === 'move' || e === 'upDown') this.show().clearSelection();
  });
  _.escapeDir = function(dir, key, e) {
    prayDirection(dir);
    var cursor = this.cursor;

    // only prevent default of Tab if not in the root editable
    if (cursor.parent !== this.root) e.preventDefault();

    // want to be a noop if in the root editable (in fact, Tab has an unrelated
    // default browser action if so)
    if (cursor.parent === this.root) return;

    cursor.parent.moveOutOf(dir, cursor);
    return this.notify('move');
  };

  optionProcessors.leftRightIntoCmdGoes = function(updown) {
    if (updown && updown !== 'up' && updown !== 'down') {
      throw '"up" or "down" required for leftRightIntoCmdGoes option, '
            + 'got "'+updown+'"';
    }
    return updown;
  };
  _.moveDir = function(dir) {
    prayDirection(dir);
    var cursor = this.cursor, updown = cursor.options.leftRightIntoCmdGoes;

    if (cursor.selection) {
      cursor.insDirOf(dir, cursor.selection.ends[dir]);
    }
    else if (cursor[dir]) cursor[dir].moveTowards(dir, cursor, updown);
    else cursor.parent.moveOutOf(dir, cursor, updown);

    return this.notify('move');
  };
  _.moveLeft = function() { return this.moveDir(L); };
  _.moveRight = function() { return this.moveDir(R); };

  /**
   * moveUp and moveDown have almost identical algorithms:
   * - first check left and right, if so insAtLeft/RightEnd of them
   * - else check the parent's 'upOutOf'/'downOutOf' property:
   *   + if it's a function, call it with the cursor as the sole argument and
   *     use the return value as if it were the value of the property
   *   + if it's a Node, jump up or down into it:
   *     - if there is a cached Point in the block, insert there
   *     - else, seekHoriz within the block to the current x-coordinate (to be
   *       as close to directly above/below the current position as possible)
   *   + unless it's exactly `true`, stop bubbling
   */
  _.moveUp = function() { return moveUpDown(this, 'up'); };
  _.moveDown = function() { return moveUpDown(this, 'down'); };
  function moveUpDown(self, dir) {
    var cursor = self.notify('upDown').cursor;
    var dirInto = dir+'Into', dirOutOf = dir+'OutOf';
    if (cursor[R][dirInto]) cursor.insAtLeftEnd(cursor[R][dirInto]);
    else if (cursor[L][dirInto]) cursor.insAtRightEnd(cursor[L][dirInto]);
    else {
      cursor.parent.bubble(function(ancestor) {
        var prop = ancestor[dirOutOf];
        if (prop) {
          if (typeof prop === 'function') prop = ancestor[dirOutOf](cursor);
          if (prop instanceof Node) cursor.jumpUpDown(ancestor, prop);
          if (prop !== true) return false;
        }
      });
    }
    return self;
  }
  this.onNotify(function(e) { if (e !== 'upDown') this.upDownCache = {}; });

  this.onNotify(function(e) { if (e === 'edit') this.show().deleteSelection(); });
  _.deleteDir = function(dir) {
    prayDirection(dir);
    var cursor = this.cursor;

    var hadSelection = cursor.selection;
    this.notify('edit'); // deletes selection if present
    if (!hadSelection) {
      if (cursor[dir]) cursor[dir].deleteTowards(dir, cursor);
      else cursor.parent.deleteOutOf(dir, cursor);
    }

    if (cursor[L].siblingDeleted) cursor[L].siblingDeleted(cursor.options, R);
    if (cursor[R].siblingDeleted) cursor[R].siblingDeleted(cursor.options, L);
    cursor.parent.bubble('reflow');

    return this;
  };
  _.ctrlDeleteDir = function(dir) {
    prayDirection(dir);
    var cursor = this.cursor;
    if (!cursor[L] || cursor.selection) return ctrlr.deleteDir();

    this.notify('edit');
    Fragment(cursor.parent.ends[L], cursor[L]).remove();
    cursor.insAtDirEnd(L, cursor.parent);

    if (cursor[L].siblingDeleted) cursor[L].siblingDeleted(cursor.options, R);
    if (cursor[R].siblingDeleted) cursor[R].siblingDeleted(cursor.options, L);
    cursor.parent.bubble('reflow');

    return this;
  };
  _.backspace = function() { return this.deleteDir(L); };
  _.deleteForward = function() { return this.deleteDir(R); };

  this.onNotify(function(e) { if (e !== 'select') this.endSelection(); });
  _.selectDir = function(dir) {
    var cursor = this.notify('select').cursor, seln = cursor.selection;
    prayDirection(dir);

    if (!cursor.anticursor) cursor.startSelection();

    var node = cursor[dir];
    if (node) {
      // "if node we're selecting towards is inside selection (hence retracting)
      // and is on the *far side* of the selection (hence is only node selected)
      // and the anticursor is *inside* that node, not just on the other side"
      if (seln && seln.ends[dir] === node && cursor.anticursor[-dir] !== node) {
        node.unselectInto(dir, cursor);
      }
      else node.selectTowards(dir, cursor);
    }
    else cursor.parent.selectOutOf(dir, cursor);

    cursor.clearSelection();
    cursor.select() || cursor.show();
  };
  _.selectLeft = function() { return this.selectDir(L); };
  _.selectRight = function() { return this.selectDir(R); };
});
// Parser MathCommand
var latexMathParser = (function() {
  function commandToBlock(cmd) {
    var block = MathBlock();
    cmd.adopt(block, 0, 0);
    return block;
  }
  function joinBlocks(blocks) {
    var firstBlock = blocks[0] || MathBlock();

    for (var i = 1; i < blocks.length; i += 1) {
      blocks[i].children().adopt(firstBlock, firstBlock.ends[R], 0);
    }

    return firstBlock;
  }

  var string = Parser.string;
  var regex = Parser.regex;
  var letter = Parser.letter;
  var any = Parser.any;
  var optWhitespace = Parser.optWhitespace;
  var succeed = Parser.succeed;
  var fail = Parser.fail;

  // Parsers yielding either MathCommands, or Fragments of MathCommands
  //   (either way, something that can be adopted by a MathBlock)
  var variable = letter.map(function(c) { return Letter(c); });
  var symbol = regex(/^[^${}\\_^]/).map(function(c) { return VanillaSymbol(c); });

  var controlSequence =
    regex(/^[^\\a-eg-zA-Z]/) // hotfix #164; match MathBlock::write
    .or(string('\\').then(
      regex(/^[a-z]+/i)
      .or(regex(/^\s+/).result(' '))
      .or(any)
    )).then(function(ctrlSeq) {
      var cmdKlass = LatexCmds[ctrlSeq];

      if (cmdKlass) {
        return cmdKlass(ctrlSeq).parser();
      }
      else {
        return fail('unknown command: \\'+ctrlSeq);
      }
    })
  ;

  var command =
    controlSequence
    .or(variable)
    .or(symbol)
  ;

  // Parsers yielding MathBlocks
  var mathGroup = string('{').then(function() { return mathSequence; }).skip(string('}'));
  var mathBlock = optWhitespace.then(mathGroup.or(command.map(commandToBlock)));
  var mathSequence = mathBlock.many().map(joinBlocks).skip(optWhitespace);

  var optMathBlock =
    string('[').then(
      mathBlock.then(function(block) {
        return block.join('latex') !== ']' ? succeed(block) : fail();
      })
      .many().map(joinBlocks).skip(optWhitespace)
    ).skip(string(']'))
  ;

  var latexMath = mathSequence;

  latexMath.block = mathBlock;
  latexMath.optBlock = optMathBlock;
  return latexMath;
})();

Controller.open(function(_, super_) {
  _.exportLatex = function() {
    return this.root.latex().replace(/(\\[a-z]+) (?![a-z])/ig,'$1');
  };
  _.writeLatex = function(latex) {
    var cursor = this.notify('edit').cursor;

    var all = Parser.all;
    var eof = Parser.eof;

    var block = latexMathParser.skip(eof).or(all.result(false)).parse(latex);

    if (block && !block.isEmpty()) {
      block.children().adopt(cursor.parent, cursor[L], cursor[R]);
      var jQ = block.jQize();
      jQ.insertBefore(cursor.jQ);
      cursor[L] = block.ends[R];
      block.finalizeInsert(cursor.options, cursor);
      if (block.ends[R][R].siblingCreated) block.ends[R][R].siblingCreated(cursor.options, L);
      if (block.ends[L][L].siblingCreated) block.ends[L][L].siblingCreated(cursor.options, R);
      cursor.parent.bubble('reflow');
    }

    return this;
  };
  _.renderLatexMath = function(latex) {
    var root = this.root, cursor = this.cursor;

    var all = Parser.all;
    var eof = Parser.eof;

    var block = latexMathParser.skip(eof).or(all.result(false)).parse(latex);

    root.eachChild('postOrder', 'dispose');
    root.ends[L] = root.ends[R] = 0;

    if (block) {
      block.children().adopt(root, 0, 0);
    }

    var jQ = root.jQ;

    if (block) {
      var html = block.join('html');
      jQ.html(html);
      root.jQize(jQ.children());
      root.finalizeInsert(cursor.options);
    }
    else {
      jQ.empty();
    }

    delete cursor.selection;
    cursor.insAtRightEnd(root);
  };
  _.renderLatexText = function(latex) {
    var root = this.root, cursor = this.cursor;

    root.jQ.children().slice(1).remove();
    root.eachChild('postOrder', 'dispose');
    root.ends[L] = root.ends[R] = 0;
    delete cursor.selection;
    cursor.show().insAtRightEnd(root);

    var regex = Parser.regex;
    var string = Parser.string;
    var eof = Parser.eof;
    var all = Parser.all;

    // Parser RootMathCommand
    var mathMode = string('$').then(latexMathParser)
      // because TeX is insane, math mode doesn't necessarily
      // have to end.  So we allow for the case that math mode
      // continues to the end of the stream.
      .skip(string('$').or(eof))
      .map(function(block) {
        // HACK FIXME: this shouldn't have to have access to cursor
        var rootMathCommand = RootMathCommand(cursor);

        rootMathCommand.createBlocks();
        var rootMathBlock = rootMathCommand.ends[L];
        block.children().adopt(rootMathBlock, 0, 0);

        return rootMathCommand;
      })
    ;

    var escapedDollar = string('\\$').result('$');
    var textChar = escapedDollar.or(regex(/^[^$]/)).map(VanillaSymbol);
    var latexText = mathMode.or(textChar).many();
    var commands = latexText.skip(eof).or(all.result(false)).parse(latex);

    if (commands) {
      for (var i = 0; i < commands.length; i += 1) {
        commands[i].adopt(root, root.ends[R], 0);
      }

      root.jQize().appendTo(root.jQ);

      root.finalizeInsert(cursor.options);
    }
  };
});
/********************************************************
 * Deals with mouse events for clicking, drag-to-select
 *******************************************************/

Controller.open(function(_) {
  _.delegateMouseEvents = function() {
    var ultimateRootjQ = this.root.jQ;
    //drag-to-select event handling
    this.container.bind('mousedown.mathquill', function(e) {
      var rootjQ = $(e.target).closest('.mq-root-block');
      var root = Node.byId[rootjQ.attr(mqBlockId) || ultimateRootjQ.attr(mqBlockId)];
      var ctrlr = root.controller, cursor = ctrlr.cursor, blink = cursor.blink;
      var textareaSpan = ctrlr.textareaSpan, textarea = ctrlr.textarea;

      var target;
      function mousemove(e) { target = $(e.target); }
      function docmousemove(e) {
        if (!cursor.anticursor) cursor.startSelection();
        ctrlr.seek(target, e.pageX, e.pageY).cursor.select();
        target = undefined;
      }
      // outside rootjQ, the MathQuill node corresponding to the target (if any)
      // won't be inside this root, so don't mislead Controller::seek with it

      function mouseup(e) {
        cursor.blink = blink;
        if (!cursor.selection) {
          if (ctrlr.editable) {
            cursor.show();
          }
          else {
            textareaSpan.detach();
          }
        }

        // delete the mouse handlers now that we're not dragging anymore
        rootjQ.unbind('mousemove', mousemove);
        $(e.target.ownerDocument).unbind('mousemove', docmousemove).unbind('mouseup', mouseup);
      }

      if (ctrlr.blurred) {
        if (!ctrlr.editable) rootjQ.prepend(textareaSpan);
        textarea.focus();
      }
      e.preventDefault(); // doesn't work in IE≤8, but it's a one-line fix:
      e.target.unselectable = true; // http://jsbin.com/yagekiji/1

      cursor.blink = noop;
      ctrlr.seek($(e.target), e.pageX, e.pageY).cursor.startSelection();

      rootjQ.mousemove(mousemove);
      $(e.target.ownerDocument).mousemove(docmousemove).mouseup(mouseup);
      // listen on document not just body to not only hear about mousemove and
      // mouseup on page outside field, but even outside page, except iframes: https://github.com/mathquill/mathquill/commit/8c50028afcffcace655d8ae2049f6e02482346c5#commitcomment-6175800
    });
  }
});

Controller.open(function(_) {
  _.seek = function(target, pageX, pageY) {
    var cursor = this.notify('select').cursor;

    if (target) {
      var nodeId = target.attr(mqBlockId) || target.attr(mqCmdId);
      if (!nodeId) {
        var targetParent = target.parent();
        nodeId = targetParent.attr(mqBlockId) || targetParent.attr(mqCmdId);
      }
    }
    var node = nodeId ? Node.byId[nodeId] : this.root;
    pray('nodeId is the id of some Node that exists', node);

    // don't clear selection until after getting node from target, in case
    // target was selection span, otherwise target will have no parent and will
    // seek from root, which is less accurate (e.g. fraction)
    cursor.clearSelection().show();

    node.seek(pageX, cursor);
    this.scrollHoriz(); // before .selectFrom when mouse-selecting, so
                        // always hits no-selection case in scrollHoriz and scrolls slower
    return this;
  };
});
/***********************************************
 * Horizontal panning for editable fields that
 * overflow their width
 **********************************************/

Controller.open(function(_) {
  _.scrollHoriz = function() {
    var cursor = this.cursor, seln = cursor.selection;
    var rootRect = this.root.jQ[0].getBoundingClientRect();
    if (!seln) {
      var x = cursor.jQ[0].getBoundingClientRect().left;
      if (x > rootRect.right - 20) var scrollBy = x - (rootRect.right - 20);
      else if (x < rootRect.left + 20) var scrollBy = x - (rootRect.left + 20);
      else return;
    }
    else {
      var rect = seln.jQ[0].getBoundingClientRect();
      var overLeft = rect.left - (rootRect.left + 20);
      var overRight = rect.right - (rootRect.right - 20);
      if (seln.ends[L] === cursor[R]) {
        if (overLeft < 0) var scrollBy = overLeft;
        else if (overRight > 0) {
          if (rect.left - overRight < rootRect.left + 20) var scrollBy = overLeft;
          else var scrollBy = overRight;
        }
        else return;
      }
      else {
        if (overRight > 0) var scrollBy = overRight;
        else if (overLeft < 0) {
          if (rect.right - overLeft > rootRect.right - 20) var scrollBy = overRight;
          else var scrollBy = overLeft;
        }
        else return;
      }
    }
    this.root.jQ.stop().animate({ scrollLeft: '+=' + scrollBy}, 100);
  };
});
/*********************************************
 * Manage the MathQuill instance's textarea
 * (as owned by the Controller)
 ********************************************/

Controller.open(function(_) {
  Options.p.substituteTextarea = function() {
    return $('<textarea autocapitalize=off autocomplete=off autocorrect=off ' +
               'spellcheck=false x-palm-disable-ste-all=true />')[0];
  };
  _.createTextarea = function() {
    var textareaSpan = this.textareaSpan = $('<span class="mq-textarea"></span>'),
      textarea = this.options.substituteTextarea();
    if (!textarea.nodeType) {
      throw 'substituteTextarea() must return a DOM element, got ' + textarea;
    }
    textarea = this.textarea = $(textarea).appendTo(textareaSpan);

    var ctrlr = this;
    ctrlr.cursor.selectionChanged = function() { ctrlr.selectionChanged(); };
    ctrlr.container.bind('copy', function() { ctrlr.setTextareaSelection(); });
  };
  _.selectionChanged = function() {
    var ctrlr = this;
    forceIERedraw(ctrlr.container[0]);

    // throttle calls to setTextareaSelection(), because setting textarea.value
    // and/or calling textarea.select() can have anomalously bad performance:
    // https://github.com/mathquill/mathquill/issues/43#issuecomment-1399080
    if (ctrlr.textareaSelectionTimeout === undefined) {
      ctrlr.textareaSelectionTimeout = setTimeout(function() {
        ctrlr.setTextareaSelection();
      });
    }
  };
  _.setTextareaSelection = function() {
    this.textareaSelectionTimeout = undefined;
    var latex = '';
    if (this.cursor.selection) {
      latex = this.cursor.selection.join('latex');
      if (this.options.statelessClipboard) {
        // FIXME: like paste, only this works for math fields; should ask parent
        latex = '$' + latex + '$';
      }
    }
    this.selectFn(latex);
  };
  _.staticMathTextareaEvents = function() {
    var ctrlr = this, root = ctrlr.root, cursor = ctrlr.cursor,
      textarea = ctrlr.textarea, textareaSpan = ctrlr.textareaSpan;

    this.container.prepend('<span class="mq-selectable">$'+ctrlr.exportLatex()+'$</span>');
    ctrlr.blurred = true;
    textarea.bind('cut paste', false)
    .focus(function() { ctrlr.blurred = false; }).blur(function() {
      if (cursor.selection) cursor.selection.clear();
      setTimeout(detach); //detaching during blur explodes in WebKit
    });
    function detach() {
      textareaSpan.detach();
      ctrlr.blurred = true;
    }

    ctrlr.selectFn = function(text) {
      textarea.val(text);
      if (text) textarea.select();
    };
  };
  _.editablesTextareaEvents = function() {
    var ctrlr = this, root = ctrlr.root, cursor = ctrlr.cursor,
      textarea = ctrlr.textarea, textareaSpan = ctrlr.textareaSpan;

    var keyboardEventsShim = saneKeyboardEvents(textarea, this);
    this.selectFn = function(text) { keyboardEventsShim.select(text); };

    this.container.prepend(textareaSpan)
    .on('cut', function(e) {
      if (cursor.selection) {
        setTimeout(function() {
          ctrlr.notify('edit'); // deletes selection if present
          cursor.parent.bubble('reflow');
        });
      }
    });

    this.focusBlurEvents();
  };
  _.typedText = function(ch) {
    if (ch === '\n') return this.handle('enter');
    var cursor = this.notify().cursor;
    cursor.parent.write(cursor, ch);
    this.scrollHoriz();
  };
  _.paste = function(text) {
    // TODO: document `statelessClipboard` config option in README, after
    // making it work like it should, that is, in both text and math mode
    // (currently only works in math fields, so worse than pointless, it
    //  only gets in the way by \text{}-ifying pasted stuff and $-ifying
    //  cut/copied LaTeX)
    if (this.options.statelessClipboard) {
      if (text.slice(0,1) === '$' && text.slice(-1) === '$') {
        text = text.slice(1, -1);
      }
      else {
        text = '\\text{'+text+'}';
      }
    }
    // FIXME: this always inserts math or a TextBlock, even in a RootTextBlock
    this.writeLatex(text).cursor.show();
  };
});
/*************************************************
 * Abstract classes of math blocks and commands.
 ************************************************/

/**
 * Math tree node base class.
 * Some math-tree-specific extensions to Node.
 * Both MathBlock's and MathCommand's descend from it.
 */
var MathElement = P(Node, function(_, super_) {
  _.finalizeInsert = function(options, cursor) { // `cursor` param is only for
      // SupSub::contactWeld, and is deliberately only passed in by writeLatex,
      // see ea7307eb4fac77c149a11ffdf9a831df85247693
    var self = this;
    self.postOrder('finalizeTree', options);
    self.postOrder('contactWeld', cursor);

    // note: this order is important.
    // empty elements need the empty box provided by blur to
    // be present in order for their dimensions to be measured
    // correctly by 'reflow' handlers.
    self.postOrder('blur');

    self.postOrder('reflow');
    if (self[R].siblingCreated) self[R].siblingCreated(options, L);
    if (self[L].siblingCreated) self[L].siblingCreated(options, R);
    self.bubble('reflow');
  };
});

/**
 * Commands and operators, like subscripts, exponents, or fractions.
 * Descendant commands are organized into blocks.
 */
var MathCommand = P(MathElement, function(_, super_) {
  _.init = function(ctrlSeq, htmlTemplate, textTemplate) {
    var cmd = this;
    super_.init.call(cmd);

    if (!cmd.ctrlSeq) cmd.ctrlSeq = ctrlSeq;
    if (htmlTemplate) cmd.htmlTemplate = htmlTemplate;
    if (textTemplate) cmd.textTemplate = textTemplate;
  };

  // obvious methods
  _.replaces = function(replacedFragment) {
    replacedFragment.disown();
    this.replacedFragment = replacedFragment;
  };
  _.isEmpty = function() {
    return this.foldChildren(true, function(isEmpty, child) {
      return isEmpty && child.isEmpty();
    });
  };

  _.parser = function() {
    var block = latexMathParser.block;
    var self = this;

    return block.times(self.numBlocks()).map(function(blocks) {
      self.blocks = blocks;

      for (var i = 0; i < blocks.length; i += 1) {
        blocks[i].adopt(self, self.ends[R], 0);
      }

      return self;
    });
  };

  // createLeftOf(cursor) and the methods it calls
  _.createLeftOf = function(cursor) {
    var cmd = this;
    var replacedFragment = cmd.replacedFragment;

    cmd.createBlocks();
    super_.createLeftOf.call(cmd, cursor);
    if (replacedFragment) {
      replacedFragment.adopt(cmd.ends[L], 0, 0);
      replacedFragment.jQ.appendTo(cmd.ends[L].jQ);
    }
    cmd.finalizeInsert(cursor.options);
    cmd.placeCursor(cursor);
  };
  _.createBlocks = function() {
    var cmd = this,
      numBlocks = cmd.numBlocks(),
      blocks = cmd.blocks = Array(numBlocks);

    for (var i = 0; i < numBlocks; i += 1) {
      var newBlock = blocks[i] = MathBlock();
      newBlock.adopt(cmd, cmd.ends[R], 0);
    }
  };
  _.placeCursor = function(cursor) {
    //insert the cursor at the right end of the first empty child, searching
    //left-to-right, or if none empty, the right end child
    cursor.insAtRightEnd(this.foldChildren(this.ends[L], function(leftward, child) {
      return leftward.isEmpty() ? leftward : child;
    }));
  };

  // editability methods: called by the cursor for editing, cursor movements,
  // and selection of the MathQuill tree, these all take in a direction and
  // the cursor
  _.moveTowards = function(dir, cursor, updown) {
    var updownInto = updown && this[updown+'Into'];
    cursor.insAtDirEnd(-dir, updownInto || this.ends[-dir]);
  };
  _.deleteTowards = function(dir, cursor) {
    if (this.isEmpty()) cursor[dir] = this.remove()[dir];
    else this.moveTowards(dir, cursor, null);
  };
  _.selectTowards = function(dir, cursor) {
    cursor[-dir] = this;
    cursor[dir] = this[dir];
  };
  _.selectChildren = function() {
    return Selection(this, this);
  };
  _.unselectInto = function(dir, cursor) {
    cursor.insAtDirEnd(-dir, cursor.anticursor.ancestors[this.id]);
  };
  _.seek = function(pageX, cursor) {
    function getBounds(node) {
      var bounds = {}
      bounds[L] = node.jQ.offset().left;
      bounds[R] = bounds[L] + node.jQ.outerWidth();
      return bounds;
    }

    var cmd = this;
    var cmdBounds = getBounds(cmd);

    if (pageX < cmdBounds[L]) return cursor.insLeftOf(cmd);
    if (pageX > cmdBounds[R]) return cursor.insRightOf(cmd);

    var leftLeftBound = cmdBounds[L];
    cmd.eachChild(function(block) {
      var blockBounds = getBounds(block);
      if (pageX < blockBounds[L]) {
        // closer to this block's left bound, or the bound left of that?
        if (pageX - leftLeftBound < blockBounds[L] - pageX) {
          if (block[L]) cursor.insAtRightEnd(block[L]);
          else cursor.insLeftOf(cmd);
        }
        else cursor.insAtLeftEnd(block);
        return false;
      }
      else if (pageX > blockBounds[R]) {
        if (block[R]) leftLeftBound = blockBounds[R]; // continue to next block
        else { // last (rightmost) block
          // closer to this block's right bound, or the cmd's right bound?
          if (cmdBounds[R] - pageX < pageX - blockBounds[R]) {
            cursor.insRightOf(cmd);
          }
          else cursor.insAtRightEnd(block);
        }
      }
      else {
        block.seek(pageX, cursor);
        return false;
      }
    });
  }

  // methods involved in creating and cross-linking with HTML DOM nodes
  /*
    They all expect an .htmlTemplate like
      '<span>&0</span>'
    or
      '<span><span>&0</span><span>&1</span></span>'

    See html.test.js for more examples.

    Requirements:
    - For each block of the command, there must be exactly one "block content
      marker" of the form '&<number>' where <number> is the 0-based index of the
      block. (Like the LaTeX \newcommand syntax, but with a 0-based rather than
      1-based index, because JavaScript because C because Dijkstra.)
    - The block content marker must be the sole contents of the containing
      element, there can't even be surrounding whitespace, or else we can't
      guarantee sticking to within the bounds of the block content marker when
      mucking with the HTML DOM.
    - The HTML not only must be well-formed HTML (of course), but also must
      conform to the XHTML requirements on tags, specifically all tags must
      either be self-closing (like '<br/>') or come in matching pairs.
      Close tags are never optional.

    Note that &<number> isn't well-formed HTML; if you wanted a literal '&123',
    your HTML template would have to have '&amp;123'.
  */
  _.numBlocks = function() {
    var matches = this.htmlTemplate.match(/&\d+/g);
    return matches ? matches.length : 0;
  };
  _.html = function() {
    // Render the entire math subtree rooted at this command, as HTML.
    // Expects .createBlocks() to have been called already, since it uses the
    // .blocks array of child blocks.
    //
    // See html.test.js for example templates and intended outputs.
    //
    // Given an .htmlTemplate as described above,
    // - insert the mathquill-command-id attribute into all top-level tags,
    //   which will be used to set this.jQ in .jQize().
    //   This is straightforward:
    //     * tokenize into tags and non-tags
    //     * loop through top-level tokens:
    //         * add #cmdId attribute macro to top-level self-closing tags
    //         * else add #cmdId attribute macro to top-level open tags
    //             * skip the matching top-level close tag and all tag pairs
    //               in between
    // - for each block content marker,
    //     + replace it with the contents of the corresponding block,
    //       rendered as HTML
    //     + insert the mathquill-block-id attribute into the containing tag
    //   This is even easier, a quick regex replace, since block tags cannot
    //   contain anything besides the block content marker.
    //
    // Two notes:
    // - The outermost loop through top-level tokens should never encounter any
    //   top-level close tags, because we should have first encountered a
    //   matching top-level open tag, all inner tags should have appeared in
    //   matching pairs and been skipped, and then we should have skipped the
    //   close tag in question.
    // - All open tags should have matching close tags, which means our inner
    //   loop should always encounter a close tag and drop nesting to 0. If
    //   a close tag is missing, the loop will continue until i >= tokens.length
    //   and token becomes undefined. This will not infinite loop, even in
    //   production without pray(), because it will then TypeError on .slice().

    var cmd = this;
    var blocks = cmd.blocks;
    var cmdId = ' mathquill-command-id=' + cmd.id;
    var tokens = cmd.htmlTemplate.match(/<[^<>]+>|[^<>]+/g);

    pray('no unmatched angle brackets', tokens.join('') === this.htmlTemplate);

    // add cmdId to all top-level tags
    for (var i = 0, token = tokens[0]; token; i += 1, token = tokens[i]) {
      // top-level self-closing tags
      if (token.slice(-2) === '/>') {
        tokens[i] = token.slice(0,-2) + cmdId + '/>';
      }
      // top-level open tags
      else if (token.charAt(0) === '<') {
        pray('not an unmatched top-level close tag', token.charAt(1) !== '/');

        tokens[i] = token.slice(0,-1) + cmdId + '>';

        // skip matching top-level close tag and all tag pairs in between
        var nesting = 1;
        do {
          i += 1, token = tokens[i];
          pray('no missing close tags', token);
          // close tags
          if (token.slice(0,2) === '</') {
            nesting -= 1;
          }
          // non-self-closing open tags
          else if (token.charAt(0) === '<' && token.slice(-2) !== '/>') {
            nesting += 1;
          }
        } while (nesting > 0);
      }
    }
    return tokens.join('').replace(/>&(\d+)/g, function($0, $1) {
      return ' mathquill-block-id=' + blocks[$1].id + '>' + blocks[$1].join('html');
    });
  };

  // methods to export a string representation of the math tree
  _.latex = function() {
    return this.foldChildren(this.ctrlSeq, function(latex, child) {
      return latex + '{' + (child.latex() || ' ') + '}';
    });
  };
  _.textTemplate = [''];
  _.text = function() {
    var cmd = this, i = 0;
    return cmd.foldChildren(cmd.textTemplate[i], function(text, child) {
      i += 1;
      var child_text = child.text();
      if (text && cmd.textTemplate[i] === '('
          && child_text[0] === '(' && child_text.slice(-1) === ')')
        return text + child_text.slice(1, -1) + cmd.textTemplate[i];
      return text + child.text() + (cmd.textTemplate[i] || '');
    });
  };
});

/**
 * Lightweight command without blocks or children.
 */
var Symbol = P(MathCommand, function(_, super_) {
  _.init = function(ctrlSeq, html, text) {
    if (!text) text = ctrlSeq && ctrlSeq.length > 1 ? ctrlSeq.slice(1) : ctrlSeq;

    super_.init.call(this, ctrlSeq, html, [ text ]);
  };

  _.parser = function() { return Parser.succeed(this); };
  _.numBlocks = function() { return 0; };

  _.replaces = function(replacedFragment) {
    replacedFragment.remove();
  };
  _.createBlocks = noop;

  _.moveTowards = function(dir, cursor) {
    cursor.jQ.insDirOf(dir, this.jQ);
    cursor[-dir] = this;
    cursor[dir] = this[dir];
  };
  _.deleteTowards = function(dir, cursor) {
    cursor[dir] = this.remove()[dir];
  };
  _.seek = function(pageX, cursor) {
    // insert at whichever side the click was closer to
    if (pageX - this.jQ.offset().left < this.jQ.outerWidth()/2)
      cursor.insLeftOf(this);
    else
      cursor.insRightOf(this);
  };

  _.latex = function(){ return this.ctrlSeq; };
  _.text = function(){ return this.textTemplate; };
  _.placeCursor = noop;
  _.isEmpty = function(){ return true; };
});
var VanillaSymbol = P(Symbol, function(_, super_) {
  _.init = function(ch, html) {
    super_.init.call(this, ch, '<span>'+(html || ch)+'</span>');
  };
});
var BinaryOperator = P(Symbol, function(_, super_) {
  _.init = function(ctrlSeq, html, text) {
    super_.init.call(this,
      ctrlSeq, '<span class="mq-binary-operator">'+html+'</span>', text
    );
  };
});

/**
 * Children and parent of MathCommand's. Basically partitions all the
 * symbols and operators that descend (in the Math DOM tree) from
 * ancestor operators.
 */
var MathBlock = P(MathElement, function(_, super_) {
  _.join = function(methodName) {
    return this.foldChildren('', function(fold, child) {
      return fold + child[methodName]();
    });
  };
  _.html = function() { return this.join('html'); };
  _.latex = function() { return this.join('latex'); };
  _.text = function() {
    return (this.ends[L] === this.ends[R] && this.ends[L] !== 0) ?
      this.ends[L].text() :
      this.join('text')
    ;
  };

  _.keystroke = function(key, e, ctrlr) {
    if (ctrlr.options.spaceBehavesLikeTab
        && (key === 'Spacebar' || key === 'Shift-Spacebar')) {
      e.preventDefault();
      ctrlr.escapeDir(key === 'Shift-Spacebar' ? L : R, key, e);
      return;
    }
    return super_.keystroke.apply(this, arguments);
  };

  // editability methods: called by the cursor for editing, cursor movements,
  // and selection of the MathQuill tree, these all take in a direction and
  // the cursor
  _.moveOutOf = function(dir, cursor, updown) {
    var updownInto = updown && this.parent[updown+'Into'];
    if (!updownInto && this[dir]) cursor.insAtDirEnd(-dir, this[dir]);
    else cursor.insDirOf(dir, this.parent);
  };
  _.selectOutOf = function(dir, cursor) {
    cursor.insDirOf(dir, this.parent);
  };
  _.deleteOutOf = function(dir, cursor) {
    cursor.unwrapGramp();
  };
  _.seek = function(pageX, cursor) {
    var node = this.ends[R];
    if (!node || node.jQ.offset().left + node.jQ.outerWidth() < pageX) {
      return cursor.insAtRightEnd(this);
    }
    if (pageX < this.ends[L].jQ.offset().left) return cursor.insAtLeftEnd(this);
    while (pageX < node.jQ.offset().left) node = node[L];
    return node.seek(pageX, cursor);
  };
  _.chToCmd = function(ch) {
    var cons;
    // exclude f because it gets a dedicated command with more spacing
    if (ch.match(/^[a-eg-zA-Z]$/))
      return Letter(ch);
    else if (/^\d$/.test(ch))
      return Digit(ch);
    else if (cons = CharCmds[ch] || LatexCmds[ch])
      return cons(ch);
    else
      return VanillaSymbol(ch);
  };
  _.write = function(cursor, ch) {
    var cmd = this.chToCmd(ch);
    if (cursor.selection) cmd.replaces(cursor.replaceSelection());
    cmd.createLeftOf(cursor.show());
  };

  _.focus = function() {
    this.jQ.addClass('mq-hasCursor');
    this.jQ.removeClass('mq-empty');

    return this;
  };
  _.blur = function() {
    this.jQ.removeClass('mq-hasCursor');
    if (this.isEmpty())
      this.jQ.addClass('mq-empty');

    return this;
  };
});

API.StaticMath = function(APIClasses) {
  return P(APIClasses.AbstractMathQuill, function(_, super_) {
    this.RootBlock = MathBlock;
    _.__mathquillify = function() {
      super_.__mathquillify.call(this, 'mq-math-mode');
      this.__controller.delegateMouseEvents();
      this.__controller.staticMathTextareaEvents();
      return this;
    };
    _.init = function() {
      super_.init.apply(this, arguments);
      this.__controller.root.postOrder(
        'registerInnerField', this.innerFields = [], APIClasses.MathField);
    };
    _.latex = function() {
      var returned = super_.latex.apply(this, arguments);
      if (arguments.length > 0) {
        this.__controller.root.postOrder(
          'registerInnerField', this.innerFields = [], APIClasses.MathField);
      }
      return returned;
    };
  });
};

var RootMathBlock = P(MathBlock, RootBlockMixin);
API.MathField = function(APIClasses) {
  return P(APIClasses.EditableField, function(_, super_) {
    this.RootBlock = RootMathBlock;
    _.__mathquillify = function(opts, interfaceVersion) {
      this.config(opts);
      if (interfaceVersion > 1) this.__controller.root.reflow = noop;
      super_.__mathquillify.call(this, 'mq-editable-field mq-math-mode');
      delete this.__controller.root.reflow;
      return this;
    };
  });
};
/*************************************************
 * Abstract classes of text blocks
 ************************************************/

/**
 * Blocks of plain text, with one or two TextPiece's as children.
 * Represents flat strings of typically serif-font Roman characters, as
 * opposed to hierchical, nested, tree-structured math.
 * Wraps a single HTMLSpanElement.
 */
var TextBlock = P(Node, function(_, super_) {
  _.ctrlSeq = '\\text';

  _.replaces = function(replacedText) {
    if (replacedText instanceof Fragment)
      this.replacedText = replacedText.remove().jQ.text();
    else if (typeof replacedText === 'string')
      this.replacedText = replacedText;
  };

  _.jQadd = function(jQ) {
    super_.jQadd.call(this, jQ);
    if (this.ends[L]) this.ends[L].jQadd(this.jQ[0].firstChild);
  };

  _.createLeftOf = function(cursor) {
    var textBlock = this;
    super_.createLeftOf.call(this, cursor);

    if (textBlock[R].siblingCreated) textBlock[R].siblingCreated(cursor.options, L);
    if (textBlock[L].siblingCreated) textBlock[L].siblingCreated(cursor.options, R);
    textBlock.bubble('reflow');

    cursor.insAtRightEnd(textBlock);

    if (textBlock.replacedText)
      for (var i = 0; i < textBlock.replacedText.length; i += 1)
        textBlock.write(cursor, textBlock.replacedText.charAt(i));
  };

  _.parser = function() {
    var textBlock = this;

    // TODO: correctly parse text mode
    var string = Parser.string;
    var regex = Parser.regex;
    var optWhitespace = Parser.optWhitespace;
    return optWhitespace
      .then(string('{')).then(regex(/^[^}]*/)).skip(string('}'))
      .map(function(text) {
        // TODO: is this the correct behavior when parsing
        // the latex \text{} ?  This violates the requirement that
        // the text contents are always nonempty.  Should we just
        // disown the parent node instead?
        TextPiece(text).adopt(textBlock, 0, 0);
        return textBlock;
      })
    ;
  };

  _.textContents = function() {
    return this.foldChildren('', function(text, child) {
      return text + child.text;
    });
  };
  _.text = function() { return '"' + this.textContents() + '"'; };
  _.latex = function() { return '\\text{' + this.textContents() + '}'; };
  _.html = function() {
    return (
        '<span class="mq-text-mode" mathquill-command-id='+this.id+'>'
      +   this.textContents()
      + '</span>'
    );
  };

  // editability methods: called by the cursor for editing, cursor movements,
  // and selection of the MathQuill tree, these all take in a direction and
  // the cursor
  _.moveTowards = function(dir, cursor) { cursor.insAtDirEnd(-dir, this); };
  _.moveOutOf = function(dir, cursor) { cursor.insDirOf(dir, this); };
  _.unselectInto = _.moveTowards;

  // TODO: make these methods part of a shared mixin or something.
  _.selectTowards = MathCommand.prototype.selectTowards;
  _.deleteTowards = MathCommand.prototype.deleteTowards;

  _.selectOutOf = function(dir, cursor) {
    cursor.insDirOf(dir, this);
  };
  _.deleteOutOf = function(dir, cursor) {
    // backspace and delete at ends of block don't unwrap
    if (this.isEmpty()) cursor.insRightOf(this);
  };
  _.write = function(cursor, ch) {
    cursor.show().deleteSelection();

    if (ch !== '$') {
      if (!cursor[L]) TextPiece(ch).createLeftOf(cursor);
      else cursor[L].appendText(ch);
    }
    else if (this.isEmpty()) {
      cursor.insRightOf(this);
      VanillaSymbol('\\$','$').createLeftOf(cursor);
    }
    else if (!cursor[R]) cursor.insRightOf(this);
    else if (!cursor[L]) cursor.insLeftOf(this);
    else { // split apart
      var leftBlock = TextBlock();
      var leftPc = this.ends[L];
      leftPc.disown();
      leftPc.adopt(leftBlock, 0, 0);

      cursor.insLeftOf(this);
      super_.createLeftOf.call(leftBlock, cursor);
    }
  };

  _.seek = function(pageX, cursor) {
    cursor.hide();
    var textPc = fuseChildren(this);

    // insert cursor at approx position in DOMTextNode
    var avgChWidth = this.jQ.width()/this.text.length;
    var approxPosition = Math.round((pageX - this.jQ.offset().left)/avgChWidth);
    if (approxPosition <= 0) cursor.insAtLeftEnd(this);
    else if (approxPosition >= textPc.text.length) cursor.insAtRightEnd(this);
    else cursor.insLeftOf(textPc.splitRight(approxPosition));

    // move towards mousedown (pageX)
    var displ = pageX - cursor.show().offset().left; // displacement
    var dir = displ && displ < 0 ? L : R;
    var prevDispl = dir;
    // displ * prevDispl > 0 iff displacement direction === previous direction
    while (cursor[dir] && displ * prevDispl > 0) {
      cursor[dir].moveTowards(dir, cursor);
      prevDispl = displ;
      displ = pageX - cursor.offset().left;
    }
    if (dir*displ < -dir*prevDispl) cursor[-dir].moveTowards(-dir, cursor);

    if (!cursor.anticursor) {
      // about to start mouse-selecting, the anticursor is gonna get put here
      this.anticursorPosition = cursor[L] && cursor[L].text.length;
      // ^ get it? 'cos if there's no cursor[L], it's 0... I'm a terrible person.
    }
    else if (cursor.anticursor.parent === this) {
      // mouse-selecting within this TextBlock, re-insert the anticursor
      var cursorPosition = cursor[L] && cursor[L].text.length;;
      if (this.anticursorPosition === cursorPosition) {
        cursor.anticursor = Point.copy(cursor);
      }
      else {
        if (this.anticursorPosition < cursorPosition) {
          var newTextPc = cursor[L].splitRight(this.anticursorPosition);
          cursor[L] = newTextPc;
        }
        else {
          var newTextPc = cursor[R].splitRight(this.anticursorPosition - cursorPosition);
        }
        cursor.anticursor = Point(this, newTextPc[L], newTextPc);
      }
    }
  };

  _.blur = function() {
    MathBlock.prototype.blur.call(this);
    fuseChildren(this);
  };

  function fuseChildren(self) {
    self.jQ[0].normalize();

    var textPcDom = self.jQ[0].firstChild;
    pray('only node in TextBlock span is Text node', textPcDom.nodeType === 3);
    // nodeType === 3 has meant a Text node since ancient times:
    //   http://reference.sitepoint.com/javascript/Node/nodeType

    var textPc = TextPiece(textPcDom.data);
    textPc.jQadd(textPcDom);

    self.children().disown();
    return textPc.adopt(self, 0, 0);
  }

  _.focus = MathBlock.prototype.focus;
});

/**
 * Piece of plain text, with a TextBlock as a parent and no children.
 * Wraps a single DOMTextNode.
 * For convenience, has a .text property that's just a JavaScript string
 * mirroring the text contents of the DOMTextNode.
 * Text contents must always be nonempty.
 */
var TextPiece = P(Node, function(_, super_) {
  _.init = function(text) {
    super_.init.call(this);
    this.text = text;
  };
  _.jQadd = function(dom) { this.dom = dom; this.jQ = $(dom); };
  _.jQize = function() {
    return this.jQadd(document.createTextNode(this.text));
  };
  _.appendText = function(text) {
    this.text += text;
    this.dom.appendData(text);
  };
  _.prependText = function(text) {
    this.text = text + this.text;
    this.dom.insertData(0, text);
  };
  _.insTextAtDirEnd = function(text, dir) {
    prayDirection(dir);
    if (dir === R) this.appendText(text);
    else this.prependText(text);
  };
  _.splitRight = function(i) {
    var newPc = TextPiece(this.text.slice(i)).adopt(this.parent, this, this[R]);
    newPc.jQadd(this.dom.splitText(i));
    this.text = this.text.slice(0, i);
    return newPc;
  };

  function endChar(dir, text) {
    return text.charAt(dir === L ? 0 : -1 + text.length);
  }

  _.moveTowards = function(dir, cursor) {
    prayDirection(dir);

    var ch = endChar(-dir, this.text)

    var from = this[-dir];
    if (from) from.insTextAtDirEnd(ch, dir);
    else TextPiece(ch).createDir(-dir, cursor);

    return this.deleteTowards(dir, cursor);
  };

  _.latex = function() { return this.text; };

  _.deleteTowards = function(dir, cursor) {
    if (this.text.length > 1) {
      if (dir === R) {
        this.dom.deleteData(0, 1);
        this.text = this.text.slice(1);
      }
      else {
        // note that the order of these 2 lines is annoyingly important
        // (the second line mutates this.text.length)
        this.dom.deleteData(-1 + this.text.length, 1);
        this.text = this.text.slice(0, -1);
      }
    }
    else {
      this.remove();
      this.jQ.remove();
      cursor[dir] = this[dir];
    }
  };

  _.selectTowards = function(dir, cursor) {
    prayDirection(dir);
    var anticursor = cursor.anticursor;

    var ch = endChar(-dir, this.text)

    if (anticursor[dir] === this) {
      var newPc = TextPiece(ch).createDir(dir, cursor);
      anticursor[dir] = newPc;
      cursor.insDirOf(dir, newPc);
    }
    else {
      var from = this[-dir];
      if (from) from.insTextAtDirEnd(ch, dir);
      else {
        var newPc = TextPiece(ch).createDir(-dir, cursor);
        newPc.jQ.insDirOf(-dir, cursor.selection.jQ);
      }

      if (this.text.length === 1 && anticursor[-dir] === this) {
        anticursor[-dir] = this[-dir]; // `this` will be removed in deleteTowards
      }
    }

    return this.deleteTowards(dir, cursor);
  };
});

CharCmds.$ =
LatexCmds.text =
LatexCmds.textnormal =
LatexCmds.textrm =
LatexCmds.textup =
LatexCmds.textmd = TextBlock;

function makeTextBlock(latex, tagName, attrs) {
  return P(TextBlock, {
    ctrlSeq: latex,
    htmlTemplate: '<'+tagName+' '+attrs+'>&0</'+tagName+'>'
  });
}

LatexCmds.em = LatexCmds.italic = LatexCmds.italics =
LatexCmds.emph = LatexCmds.textit = LatexCmds.textsl =
  makeTextBlock('\\textit', 'i', 'class="mq-text-mode"');
LatexCmds.strong = LatexCmds.bold = LatexCmds.textbf =
  makeTextBlock('\\textbf', 'b', 'class="mq-text-mode"');
LatexCmds.sf = LatexCmds.textsf =
  makeTextBlock('\\textsf', 'span', 'class="mq-sans-serif mq-text-mode"');
LatexCmds.tt = LatexCmds.texttt =
  makeTextBlock('\\texttt', 'span', 'class="mq-monospace mq-text-mode"');
LatexCmds.textsc =
  makeTextBlock('\\textsc', 'span', 'style="font-variant:small-caps" class="mq-text-mode"');
LatexCmds.uppercase =
  makeTextBlock('\\uppercase', 'span', 'style="text-transform:uppercase" class="mq-text-mode"');
LatexCmds.lowercase =
  makeTextBlock('\\lowercase', 'span', 'style="text-transform:lowercase" class="mq-text-mode"');


var RootMathCommand = P(MathCommand, function(_, super_) {
  _.init = function(cursor) {
    super_.init.call(this, '$');
    this.cursor = cursor;
  };
  _.htmlTemplate = '<span class="mq-math-mode">&0</span>';
  _.createBlocks = function() {
    super_.createBlocks.call(this);

    this.ends[L].cursor = this.cursor;
    this.ends[L].write = function(cursor, ch) {
      if (ch !== '$')
        MathBlock.prototype.write.call(this, cursor, ch);
      else if (this.isEmpty()) {
        cursor.insRightOf(this.parent);
        this.parent.deleteTowards(dir, cursor);
        VanillaSymbol('\\$','$').createLeftOf(cursor.show());
      }
      else if (!cursor[R])
        cursor.insRightOf(this.parent);
      else if (!cursor[L])
        cursor.insLeftOf(this.parent);
      else
        MathBlock.prototype.write.call(this, cursor, ch);
    };
  };
  _.latex = function() {
    return '$' + this.ends[L].latex() + '$';
  };
});

var RootTextBlock = P(RootMathBlock, function(_, super_) {
  _.keystroke = function(key) {
    if (key === 'Spacebar' || key === 'Shift-Spacebar') return;
    return super_.keystroke.apply(this, arguments);
  };
  _.write = function(cursor, ch) {
    cursor.show().deleteSelection();
    if (ch === '$')
      RootMathCommand(cursor).createLeftOf(cursor);
    else {
      var html;
      if (ch === '<') html = '&lt;';
      else if (ch === '>') html = '&gt;';
      VanillaSymbol(ch, html).createLeftOf(cursor);
    }
  };
});
API.TextField = function(APIClasses) {
  return P(APIClasses.EditableField, function(_, super_) {
    this.RootBlock = RootTextBlock;
    _.__mathquillify = function() {
      return super_.__mathquillify.call(this, 'mq-editable-field mq-text-mode');
    };
    _.latex = function(latex) {
      if (arguments.length > 0) {
        this.__controller.renderLatexText(latex);
        if (this.__controller.blurred) this.__controller.cursor.hide().parent.blur();
        return this;
      }
      return this.__controller.exportLatex();
    };
  });
};
/****************************************
 * Input box to type backslash commands
 ***************************************/

var LatexCommandInput =
CharCmds['\\'] = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '\\';
  _.replaces = function(replacedFragment) {
    this._replacedFragment = replacedFragment.disown();
    this.isEmpty = function() { return false; };
  };
  _.htmlTemplate = '<span class="mq-latex-command-input mq-non-leaf">\\<span>&0</span></span>';
  _.textTemplate = ['\\'];
  _.createBlocks = function() {
    super_.createBlocks.call(this);
    this.ends[L].focus = function() {
      this.parent.jQ.addClass('mq-hasCursor');
      if (this.isEmpty())
        this.parent.jQ.removeClass('mq-empty');

      return this;
    };
    this.ends[L].blur = function() {
      this.parent.jQ.removeClass('mq-hasCursor');
      if (this.isEmpty())
        this.parent.jQ.addClass('mq-empty');

      return this;
    };
    this.ends[L].write = function(cursor, ch) {
      cursor.show().deleteSelection();

      if (ch.match(/[a-z]/i)) VanillaSymbol(ch).createLeftOf(cursor);
      else {
        this.parent.renderCommand(cursor);
        if (ch !== '\\' || !this.isEmpty()) this.parent.parent.write(cursor, ch);
      }
    };
    this.ends[L].keystroke = function(key, e, ctrlr) {
      if (key === 'Tab' || key === 'Enter' || key === 'Spacebar') {
        this.parent.renderCommand(ctrlr.cursor);
        e.preventDefault();
        return;
      }
      return super_.keystroke.apply(this, arguments);
    };
  };
  _.createLeftOf = function(cursor) {
    super_.createLeftOf.call(this, cursor);

    if (this._replacedFragment) {
      var el = this.jQ[0];
      this.jQ =
        this._replacedFragment.jQ.addClass('mq-blur').bind(
          'mousedown mousemove', //FIXME: is monkey-patching the mousedown and mousemove handlers the right way to do this?
          function(e) {
            $(e.target = el).trigger(e);
            return false;
          }
        ).insertBefore(this.jQ).add(this.jQ);
    }
  };
  _.latex = function() {
    return '\\' + this.ends[L].latex() + ' ';
  };
  _.renderCommand = function(cursor) {
    this.jQ = this.jQ.last();
    this.remove();
    if (this[R]) {
      cursor.insLeftOf(this[R]);
    } else {
      cursor.insAtRightEnd(this.parent);
    }

    var latex = this.ends[L].latex();
    if (!latex) latex = ' ';
    var cmd = LatexCmds[latex];
    if (cmd) {
      cmd = cmd(latex);
      if (this._replacedFragment) cmd.replaces(this._replacedFragment);
      cmd.createLeftOf(cursor);
    }
    else {
      cmd = TextBlock();
      cmd.replaces(latex);
      cmd.createLeftOf(cursor);
      cursor.insRightOf(cmd);
      if (this._replacedFragment)
        this._replacedFragment.remove();
    }
  };
});

/************************************
 * Symbols for Advanced Mathematics
 ***********************************/

LatexCmds.notin =
LatexCmds.cong =
LatexCmds.equiv =
LatexCmds.oplus =
LatexCmds.otimes = P(BinaryOperator, function(_, super_) {
  _.init = function(latex) {
    super_.init.call(this, '\\'+latex+' ', '&'+latex+';');
  };
});

LatexCmds['≠'] = LatexCmds.ne = LatexCmds.neq = bind(BinaryOperator,'\\ne ','&ne;');

LatexCmds.ast = LatexCmds.star = LatexCmds.loast = LatexCmds.lowast =
  bind(BinaryOperator,'\\ast ','&lowast;');
  //case 'there4 = // a special exception for this one, perhaps?
LatexCmds.therefor = LatexCmds.therefore =
  bind(BinaryOperator,'\\therefore ','&there4;');

LatexCmds.cuz = // l33t
LatexCmds.because = bind(BinaryOperator,'\\because ','&#8757;');

LatexCmds.prop = LatexCmds.propto = bind(BinaryOperator,'\\propto ','&prop;');

LatexCmds['≈'] = LatexCmds.asymp = LatexCmds.approx = bind(BinaryOperator,'\\approx ','&asymp;');

LatexCmds.isin = LatexCmds['in'] = bind(BinaryOperator,'\\in ','&isin;');

LatexCmds.ni = LatexCmds.contains = bind(BinaryOperator,'\\ni ','&ni;');

LatexCmds.notni = LatexCmds.niton = LatexCmds.notcontains = LatexCmds.doesnotcontain =
  bind(BinaryOperator,'\\not\\ni ','&#8716;');

LatexCmds.sub = LatexCmds.subset = bind(BinaryOperator,'\\subset ','&sub;');

LatexCmds.sup = LatexCmds.supset = LatexCmds.superset =
  bind(BinaryOperator,'\\supset ','&sup;');

LatexCmds.nsub = LatexCmds.notsub =
LatexCmds.nsubset = LatexCmds.notsubset =
  bind(BinaryOperator,'\\not\\subset ','&#8836;');

LatexCmds.nsup = LatexCmds.notsup =
LatexCmds.nsupset = LatexCmds.notsupset =
LatexCmds.nsuperset = LatexCmds.notsuperset =
  bind(BinaryOperator,'\\not\\supset ','&#8837;');

LatexCmds.sube = LatexCmds.subeq = LatexCmds.subsete = LatexCmds.subseteq =
  bind(BinaryOperator,'\\subseteq ','&sube;');

LatexCmds.supe = LatexCmds.supeq =
LatexCmds.supsete = LatexCmds.supseteq =
LatexCmds.supersete = LatexCmds.superseteq =
  bind(BinaryOperator,'\\supseteq ','&supe;');

LatexCmds.nsube = LatexCmds.nsubeq =
LatexCmds.notsube = LatexCmds.notsubeq =
LatexCmds.nsubsete = LatexCmds.nsubseteq =
LatexCmds.notsubsete = LatexCmds.notsubseteq =
  bind(BinaryOperator,'\\not\\subseteq ','&#8840;');

LatexCmds.nsupe = LatexCmds.nsupeq =
LatexCmds.notsupe = LatexCmds.notsupeq =
LatexCmds.nsupsete = LatexCmds.nsupseteq =
LatexCmds.notsupsete = LatexCmds.notsupseteq =
LatexCmds.nsupersete = LatexCmds.nsuperseteq =
LatexCmds.notsupersete = LatexCmds.notsuperseteq =
  bind(BinaryOperator,'\\not\\supseteq ','&#8841;');


//the canonical sets of numbers
LatexCmds.N = LatexCmds.naturals = LatexCmds.Naturals =
  bind(VanillaSymbol,'\\mathbb{N}','&#8469;');

LatexCmds.P =
LatexCmds.primes = LatexCmds.Primes =
LatexCmds.projective = LatexCmds.Projective =
LatexCmds.probability = LatexCmds.Probability =
  bind(VanillaSymbol,'\\mathbb{P}','&#8473;');

LatexCmds.Z = LatexCmds.integers = LatexCmds.Integers =
  bind(VanillaSymbol,'\\mathbb{Z}','&#8484;');

LatexCmds.Q = LatexCmds.rationals = LatexCmds.Rationals =
  bind(VanillaSymbol,'\\mathbb{Q}','&#8474;');

LatexCmds.R = LatexCmds.reals = LatexCmds.Reals =
  bind(VanillaSymbol,'\\mathbb{R}','&#8477;');

LatexCmds.C =
LatexCmds.complex = LatexCmds.Complex =
LatexCmds.complexes = LatexCmds.Complexes =
LatexCmds.complexplane = LatexCmds.Complexplane = LatexCmds.ComplexPlane =
  bind(VanillaSymbol,'\\mathbb{C}','&#8450;');

LatexCmds.H = LatexCmds.Hamiltonian = LatexCmds.quaternions = LatexCmds.Quaternions =
  bind(VanillaSymbol,'\\mathbb{H}','&#8461;');

//spacing
LatexCmds.quad = LatexCmds.emsp = bind(VanillaSymbol,'\\quad ','    ');
LatexCmds.qquad = bind(VanillaSymbol,'\\qquad ','        ');
/* spacing special characters, gonna have to implement this in LatexCommandInput::onText somehow
case ',':
  return VanillaSymbol('\\, ',' ');
case ':':
  return VanillaSymbol('\\: ','  ');
case ';':
  return VanillaSymbol('\\; ','   ');
case '!':
  return Symbol('\\! ','<span style="margin-right:-.2em"></span>');
*/

//binary operators
LatexCmds.diamond = bind(VanillaSymbol, '\\diamond ', '&#9671;');
LatexCmds.bigtriangleup = bind(VanillaSymbol, '\\bigtriangleup ', '&#9651;');
LatexCmds.ominus = bind(VanillaSymbol, '\\ominus ', '&#8854;');
LatexCmds.uplus = bind(VanillaSymbol, '\\uplus ', '&#8846;');
LatexCmds.bigtriangledown = bind(VanillaSymbol, '\\bigtriangledown ', '&#9661;');
LatexCmds.sqcap = bind(VanillaSymbol, '\\sqcap ', '&#8851;');
LatexCmds.triangleleft = bind(VanillaSymbol, '\\triangleleft ', '&#8882;');
LatexCmds.sqcup = bind(VanillaSymbol, '\\sqcup ', '&#8852;');
LatexCmds.triangleright = bind(VanillaSymbol, '\\triangleright ', '&#8883;');
//circledot is not a not real LaTex command see https://github.com/mathquill/mathquill/pull/552 for more details
LatexCmds.odot = LatexCmds.circledot = bind(VanillaSymbol, '\\odot ', '&#8857;');
LatexCmds.bigcirc = bind(VanillaSymbol, '\\bigcirc ', '&#9711;');
LatexCmds.dagger = bind(VanillaSymbol, '\\dagger ', '&#0134;');
LatexCmds.ddagger = bind(VanillaSymbol, '\\ddagger ', '&#135;');
LatexCmds.wr = bind(VanillaSymbol, '\\wr ', '&#8768;');
LatexCmds.amalg = bind(VanillaSymbol, '\\amalg ', '&#8720;');

//relationship symbols
LatexCmds.models = bind(VanillaSymbol, '\\models ', '&#8872;');
LatexCmds.prec = bind(VanillaSymbol, '\\prec ', '&#8826;');
LatexCmds.succ = bind(VanillaSymbol, '\\succ ', '&#8827;');
LatexCmds.preceq = bind(VanillaSymbol, '\\preceq ', '&#8828;');
LatexCmds.succeq = bind(VanillaSymbol, '\\succeq ', '&#8829;');
LatexCmds.simeq = bind(VanillaSymbol, '\\simeq ', '&#8771;');
LatexCmds.mid = bind(VanillaSymbol, '\\mid ', '&#8739;');
LatexCmds.ll = bind(VanillaSymbol, '\\ll ', '&#8810;');
LatexCmds.gg = bind(VanillaSymbol, '\\gg ', '&#8811;');
LatexCmds.parallel = bind(VanillaSymbol, '\\parallel ', '&#8741;');
LatexCmds.nparallel = bind(VanillaSymbol, '\\nparallel ', '&#8742;');
LatexCmds.bowtie = bind(VanillaSymbol, '\\bowtie ', '&#8904;');
LatexCmds.sqsubset = bind(VanillaSymbol, '\\sqsubset ', '&#8847;');
LatexCmds.sqsupset = bind(VanillaSymbol, '\\sqsupset ', '&#8848;');
LatexCmds.smile = bind(VanillaSymbol, '\\smile ', '&#8995;');
LatexCmds.sqsubseteq = bind(VanillaSymbol, '\\sqsubseteq ', '&#8849;');
LatexCmds.sqsupseteq = bind(VanillaSymbol, '\\sqsupseteq ', '&#8850;');
LatexCmds.doteq = bind(VanillaSymbol, '\\doteq ', '&#8784;');
LatexCmds.frown = bind(VanillaSymbol, '\\frown ', '&#8994;');
LatexCmds.vdash = bind(VanillaSymbol, '\\vdash ', '&#8870;');
LatexCmds.dashv = bind(VanillaSymbol, '\\dashv ', '&#8867;');
LatexCmds.nless = bind(VanillaSymbol, '\\nless ', '&#8814;');
LatexCmds.ngtr = bind(VanillaSymbol, '\\ngtr ', '&#8815;');

//arrows
LatexCmds.longleftarrow = bind(VanillaSymbol, '\\longleftarrow ', '&#8592;');
LatexCmds.longrightarrow = bind(VanillaSymbol, '\\longrightarrow ', '&#8594;');
LatexCmds.Longleftarrow = bind(VanillaSymbol, '\\Longleftarrow ', '&#8656;');
LatexCmds.Longrightarrow = bind(VanillaSymbol, '\\Longrightarrow ', '&#8658;');
LatexCmds.longleftrightarrow = bind(VanillaSymbol, '\\longleftrightarrow ', '&#8596;');
LatexCmds.updownarrow = bind(VanillaSymbol, '\\updownarrow ', '&#8597;');
LatexCmds.Longleftrightarrow = bind(VanillaSymbol, '\\Longleftrightarrow ', '&#8660;');
LatexCmds.Updownarrow = bind(VanillaSymbol, '\\Updownarrow ', '&#8661;');
LatexCmds.mapsto = bind(VanillaSymbol, '\\mapsto ', '&#8614;');
LatexCmds.nearrow = bind(VanillaSymbol, '\\nearrow ', '&#8599;');
LatexCmds.hookleftarrow = bind(VanillaSymbol, '\\hookleftarrow ', '&#8617;');
LatexCmds.hookrightarrow = bind(VanillaSymbol, '\\hookrightarrow ', '&#8618;');
LatexCmds.searrow = bind(VanillaSymbol, '\\searrow ', '&#8600;');
LatexCmds.leftharpoonup = bind(VanillaSymbol, '\\leftharpoonup ', '&#8636;');
LatexCmds.rightharpoonup = bind(VanillaSymbol, '\\rightharpoonup ', '&#8640;');
LatexCmds.swarrow = bind(VanillaSymbol, '\\swarrow ', '&#8601;');
LatexCmds.leftharpoondown = bind(VanillaSymbol, '\\leftharpoondown ', '&#8637;');
LatexCmds.rightharpoondown = bind(VanillaSymbol, '\\rightharpoondown ', '&#8641;');
LatexCmds.nwarrow = bind(VanillaSymbol, '\\nwarrow ', '&#8598;');

//Misc
LatexCmds.ldots = bind(VanillaSymbol, '\\ldots ', '&#8230;');
LatexCmds.cdots = bind(VanillaSymbol, '\\cdots ', '&#8943;');
LatexCmds.vdots = bind(VanillaSymbol, '\\vdots ', '&#8942;');
LatexCmds.ddots = bind(VanillaSymbol, '\\ddots ', '&#8945;');
LatexCmds.surd = bind(VanillaSymbol, '\\surd ', '&#8730;');
LatexCmds.triangle = bind(VanillaSymbol, '\\triangle ', '&#9651;');
LatexCmds.ell = bind(VanillaSymbol, '\\ell ', '&#8467;');
LatexCmds.top = bind(VanillaSymbol, '\\top ', '&#8868;');
LatexCmds.flat = bind(VanillaSymbol, '\\flat ', '&#9837;');
LatexCmds.natural = bind(VanillaSymbol, '\\natural ', '&#9838;');
LatexCmds.sharp = bind(VanillaSymbol, '\\sharp ', '&#9839;');
LatexCmds.wp = bind(VanillaSymbol, '\\wp ', '&#8472;');
LatexCmds.bot = bind(VanillaSymbol, '\\bot ', '&#8869;');
LatexCmds.clubsuit = bind(VanillaSymbol, '\\clubsuit ', '&#9827;');
LatexCmds.diamondsuit = bind(VanillaSymbol, '\\diamondsuit ', '&#9826;');
LatexCmds.heartsuit = bind(VanillaSymbol, '\\heartsuit ', '&#9825;');
LatexCmds.spadesuit = bind(VanillaSymbol, '\\spadesuit ', '&#9824;');
//not real LaTex command see https://github.com/mathquill/mathquill/pull/552 for more details
LatexCmds.parallelogram = bind(VanillaSymbol, '\\parallelogram ', '&#9649;');
LatexCmds.square = bind(VanillaSymbol, '\\square ', '&#11036;');

//variable-sized
LatexCmds.oint = bind(VanillaSymbol, '\\oint ', '&#8750;');
LatexCmds.bigcap = bind(VanillaSymbol, '\\bigcap ', '&#8745;');
LatexCmds.bigcup = bind(VanillaSymbol, '\\bigcup ', '&#8746;');
LatexCmds.bigsqcup = bind(VanillaSymbol, '\\bigsqcup ', '&#8852;');
LatexCmds.bigvee = bind(VanillaSymbol, '\\bigvee ', '&#8744;');
LatexCmds.bigwedge = bind(VanillaSymbol, '\\bigwedge ', '&#8743;');
LatexCmds.bigodot = bind(VanillaSymbol, '\\bigodot ', '&#8857;');
LatexCmds.bigotimes = bind(VanillaSymbol, '\\bigotimes ', '&#8855;');
LatexCmds.bigoplus = bind(VanillaSymbol, '\\bigoplus ', '&#8853;');
LatexCmds.biguplus = bind(VanillaSymbol, '\\biguplus ', '&#8846;');

//delimiters
LatexCmds.lfloor = bind(VanillaSymbol, '\\lfloor ', '&#8970;');
LatexCmds.rfloor = bind(VanillaSymbol, '\\rfloor ', '&#8971;');
LatexCmds.lceil = bind(VanillaSymbol, '\\lceil ', '&#8968;');
LatexCmds.rceil = bind(VanillaSymbol, '\\rceil ', '&#8969;');
LatexCmds.opencurlybrace = LatexCmds.lbrace = bind(VanillaSymbol, '\\lbrace ', '{');
LatexCmds.closecurlybrace = LatexCmds.rbrace = bind(VanillaSymbol, '\\rbrace ', '}');
LatexCmds.lbrack = bind(VanillaSymbol, '[');
LatexCmds.rbrack = bind(VanillaSymbol, ']');

//various symbols
LatexCmds['∫'] =
LatexCmds['int'] =
LatexCmds.integral = bind(Symbol,'\\int ','<big>&int;</big>');

LatexCmds.slash = bind(VanillaSymbol, '/');
LatexCmds.vert = bind(VanillaSymbol,'|');
LatexCmds.perp = LatexCmds.perpendicular = bind(VanillaSymbol,'\\perp ','&perp;');
LatexCmds.nabla = LatexCmds.del = bind(VanillaSymbol,'\\nabla ','&nabla;');
LatexCmds.hbar = bind(VanillaSymbol,'\\hbar ','&#8463;');

LatexCmds.AA = LatexCmds.Angstrom = LatexCmds.angstrom =
  bind(VanillaSymbol,'\\text\\AA ','&#8491;');

LatexCmds.ring = LatexCmds.circ = LatexCmds.circle =
  bind(VanillaSymbol,'\\circ ','&#8728;');

LatexCmds.bull = LatexCmds.bullet = bind(VanillaSymbol,'\\bullet ','&bull;');

LatexCmds.setminus = LatexCmds.smallsetminus =
  bind(VanillaSymbol,'\\setminus ','&#8726;');

LatexCmds.not = //bind(Symbol,'\\not ','<span class="not">/</span>');
LatexCmds['¬'] = LatexCmds.neg = bind(VanillaSymbol,'\\neg ','&not;');

LatexCmds['…'] = LatexCmds.dots = LatexCmds.ellip = LatexCmds.hellip =
LatexCmds.ellipsis = LatexCmds.hellipsis =
  bind(VanillaSymbol,'\\dots ','&hellip;');

LatexCmds.converges =
LatexCmds.darr = LatexCmds.dnarr = LatexCmds.dnarrow = LatexCmds.downarrow =
  bind(VanillaSymbol,'\\downarrow ','&darr;');

LatexCmds.dArr = LatexCmds.dnArr = LatexCmds.dnArrow = LatexCmds.Downarrow =
  bind(VanillaSymbol,'\\Downarrow ','&dArr;');

LatexCmds.diverges = LatexCmds.uarr = LatexCmds.uparrow =
  bind(VanillaSymbol,'\\uparrow ','&uarr;');

LatexCmds.uArr = LatexCmds.Uparrow = bind(VanillaSymbol,'\\Uparrow ','&uArr;');

LatexCmds.to = bind(BinaryOperator,'\\to ','&rarr;');

LatexCmds.rarr = LatexCmds.rightarrow = bind(VanillaSymbol,'\\rightarrow ','&rarr;');

LatexCmds.implies = bind(BinaryOperator,'\\Rightarrow ','&rArr;');

LatexCmds.rArr = LatexCmds.Rightarrow = bind(VanillaSymbol,'\\Rightarrow ','&rArr;');

LatexCmds.gets = bind(BinaryOperator,'\\gets ','&larr;');

LatexCmds.larr = LatexCmds.leftarrow = bind(VanillaSymbol,'\\leftarrow ','&larr;');

LatexCmds.impliedby = bind(BinaryOperator,'\\Leftarrow ','&lArr;');

LatexCmds.lArr = LatexCmds.Leftarrow = bind(VanillaSymbol,'\\Leftarrow ','&lArr;');

LatexCmds.harr = LatexCmds.lrarr = LatexCmds.leftrightarrow =
  bind(VanillaSymbol,'\\leftrightarrow ','&harr;');

LatexCmds.iff = bind(BinaryOperator,'\\Leftrightarrow ','&hArr;');

LatexCmds.hArr = LatexCmds.lrArr = LatexCmds.Leftrightarrow =
  bind(VanillaSymbol,'\\Leftrightarrow ','&hArr;');

LatexCmds.Re = LatexCmds.Real = LatexCmds.real = bind(VanillaSymbol,'\\Re ','&real;');

LatexCmds.Im = LatexCmds.imag =
LatexCmds.image = LatexCmds.imagin = LatexCmds.imaginary = LatexCmds.Imaginary =
  bind(VanillaSymbol,'\\Im ','&image;');

LatexCmds.part = LatexCmds.partial = bind(VanillaSymbol,'\\partial ','&part;');

LatexCmds.infty = LatexCmds.infin = LatexCmds.infinity =
  bind(VanillaSymbol,'\\infty ','&infin;');

LatexCmds.alef = LatexCmds.alefsym = LatexCmds.aleph = LatexCmds.alephsym =
  bind(VanillaSymbol,'\\aleph ','&alefsym;');

LatexCmds.xist = //LOL
LatexCmds.xists = LatexCmds.exist = LatexCmds.exists =
  bind(VanillaSymbol,'\\exists ','&exist;');

LatexCmds.and = LatexCmds.land = LatexCmds.wedge =
  bind(VanillaSymbol,'\\wedge ','&and;');

LatexCmds.or = LatexCmds.lor = LatexCmds.vee = bind(VanillaSymbol,'\\vee ','&or;');

LatexCmds.o = LatexCmds.O =
LatexCmds.empty = LatexCmds.emptyset =
LatexCmds.oslash = LatexCmds.Oslash =
LatexCmds.nothing = LatexCmds.varnothing =
  bind(BinaryOperator,'\\varnothing ','&empty;');

LatexCmds.cup = LatexCmds.union = bind(BinaryOperator,'\\cup ','&cup;');

LatexCmds.cap = LatexCmds.intersect = LatexCmds.intersection =
  bind(BinaryOperator,'\\cap ','&cap;');

// FIXME: the correct LaTeX would be ^\circ but we can't parse that
LatexCmds.deg = LatexCmds.degree = bind(VanillaSymbol,'\\degree ','&deg;');

LatexCmds.ang = LatexCmds.angle = bind(VanillaSymbol,'\\angle ','&ang;');
LatexCmds.measuredangle = bind(VanillaSymbol,'\\measuredangle ','&#8737;');
/*********************************
 * Symbols for Basic Mathematics
 ********************************/

var Digit = P(VanillaSymbol, function(_, super_) {
  _.createLeftOf = function(cursor) {
    if (cursor.options.autoSubscriptNumerals
        && cursor.parent !== cursor.parent.parent.sub
        && ((cursor[L] instanceof Variable && cursor[L].isItalic !== false)
            || (cursor[L] instanceof SupSub
                && cursor[L][L] instanceof Variable
                && cursor[L][L].isItalic !== false))) {
      LatexCmds._().createLeftOf(cursor);
      super_.createLeftOf.call(this, cursor);
      cursor.insRightOf(cursor.parent.parent);
    }
    else super_.createLeftOf.call(this, cursor);
  };
});

var Variable = P(Symbol, function(_, super_) {
  _.init = function(ch, html) {
    super_.init.call(this, ch, '<var>'+(html || ch)+'</var>');
  };
  _.text = function() {
    var text = this.ctrlSeq;
    if (this[L] && !(this[L] instanceof Variable)
        && !(this[L] instanceof BinaryOperator)
        && this[L].ctrlSeq !== "\\ ")
      text = '*' + text;
    if (this[R] && !(this[R] instanceof BinaryOperator)
        && !(this[R] instanceof SupSub))
      text += '*';
    return text;
  };
});

Options.p.autoCommands = { _maxLength: 0 };
optionProcessors.autoCommands = function(cmds) {
  if (!/^[a-z]+(?: [a-z]+)*$/i.test(cmds)) {
    throw '"'+cmds+'" not a space-delimited list of only letters';
  }
  var list = cmds.split(' '), dict = {}, maxLength = 0;
  for (var i = 0; i < list.length; i += 1) {
    var cmd = list[i];
    if (cmd.length < 2) {
      throw 'autocommand "'+cmd+'" not minimum length of 2';
    }
    if (LatexCmds[cmd] === OperatorName) {
      throw '"' + cmd + '" is a built-in operator name';
    }
    dict[cmd] = 1;
    maxLength = max(maxLength, cmd.length);
  }
  dict._maxLength = maxLength;
  return dict;
};

var Letter = P(Variable, function(_, super_) {
  _.init = function(ch) { return super_.init.call(this, this.letter = ch); };
  _.createLeftOf = function(cursor) {
    var autoCmds = cursor.options.autoCommands, maxLength = autoCmds._maxLength;
    if (maxLength > 0) {
      // want longest possible autocommand, so join together longest
      // sequence of letters
      var str = this.letter, l = cursor[L], i = 1;
      while (l instanceof Letter && i < maxLength) {
        str = l.letter + str, l = l[L], i += 1;
      }
      // check for an autocommand, going thru substrings longest to shortest
      while (str.length) {
        if (autoCmds.hasOwnProperty(str)) {
          for (var i = 2, l = cursor[L]; i < str.length; i += 1, l = l[L]);
          Fragment(l, cursor[L]).remove();
          cursor[L] = l[L];
          return LatexCmds[str](str).createLeftOf(cursor);
        }
        str = str.slice(1);
      }
    }
    super_.createLeftOf.apply(this, arguments);
  };
  _.italicize = function(bool) {
    this.isItalic = bool;
    this.jQ.toggleClass('mq-operator-name', !bool);
    return this;
  };
  _.finalizeTree = _.siblingDeleted = _.siblingCreated = function(opts, dir) {
    // don't auto-un-italicize if the sibling to my right changed (dir === R or
    // undefined) and it's now a Letter, it will un-italicize everyone
    if (dir !== L && this[R] instanceof Letter) return;
    this.autoUnItalicize(opts);
  };
  _.autoUnItalicize = function(opts) {
    var autoOps = opts.autoOperatorNames;
    if (autoOps._maxLength === 0) return;
    // want longest possible operator names, so join together entire contiguous
    // sequence of letters
    var str = this.letter;
    for (var l = this[L]; l instanceof Letter; l = l[L]) str = l.letter + str;
    for (var r = this[R]; r instanceof Letter; r = r[R]) str += r.letter;

    // removeClass and delete flags from all letters before figuring out
    // which, if any, are part of an operator name
    Fragment(l[R] || this.parent.ends[L], r[L] || this.parent.ends[R]).each(function(el) {
      el.italicize(true).jQ.removeClass('mq-first mq-last');
      el.ctrlSeq = el.letter;
    });

    // check for operator names: at each position from left to right, check
    // substrings from longest to shortest
    outer: for (var i = 0, first = l[R] || this.parent.ends[L]; i < str.length; i += 1, first = first[R]) {
      for (var len = min(autoOps._maxLength, str.length - i); len > 0; len -= 1) {
        var word = str.slice(i, i + len);
        if (autoOps.hasOwnProperty(word)) {
          for (var j = 0, letter = first; j < len; j += 1, letter = letter[R]) {
            letter.italicize(false);
            var last = letter;
          }

          var isBuiltIn = BuiltInOpNames.hasOwnProperty(word);
          first.ctrlSeq = (isBuiltIn ? '\\' : '\\operatorname{') + first.ctrlSeq;
          last.ctrlSeq += (isBuiltIn ? ' ' : '}');
          if (TwoWordOpNames.hasOwnProperty(word)) last[L][L][L].jQ.addClass('mq-last');
          if (nonOperatorSymbol(first[L])) first.jQ.addClass('mq-first');
          if (nonOperatorSymbol(last[R])) last.jQ.addClass('mq-last');

          i += len - 1;
          first = last;
          continue outer;
        }
      }
    }
  };
  function nonOperatorSymbol(node) {
    return node instanceof Symbol && !(node instanceof BinaryOperator);
  }
});
var BuiltInOpNames = {}; // the set of operator names like \sin, \cos, etc that
  // are built-into LaTeX: http://latex.wikia.com/wiki/List_of_LaTeX_symbols#Named_operators:_sin.2C_cos.2C_etc.
  // MathQuill auto-unitalicizes some operator names not in that set, like 'hcf'
  // and 'arsinh', which must be exported as \operatorname{hcf} and
  // \operatorname{arsinh}. Note: over/under line/arrow \lim variants like
  // \varlimsup are not supported
var AutoOpNames = Options.p.autoOperatorNames = { _maxLength: 9 }; // the set
  // of operator names that MathQuill auto-unitalicizes by default; overridable
var TwoWordOpNames = { limsup: 1, liminf: 1, projlim: 1, injlim: 1 };
(function() {
  var mostOps = ('arg deg det dim exp gcd hom inf ker lg lim ln log max min sup'
                 + ' limsup liminf injlim projlim Pr').split(' ');
  for (var i = 0; i < mostOps.length; i += 1) {
    BuiltInOpNames[mostOps[i]] = AutoOpNames[mostOps[i]] = 1;
  }

  var builtInTrigs = // why coth but not sech and csch, LaTeX?
    'sin cos tan arcsin arccos arctan sinh cosh tanh sec csc cot coth'.split(' ');
  for (var i = 0; i < builtInTrigs.length; i += 1) {
    BuiltInOpNames[builtInTrigs[i]] = 1;
  }

  var autoTrigs = 'sin cos tan sec cosec csc cotan cot ctg'.split(' ');
  for (var i = 0; i < autoTrigs.length; i += 1) {
    AutoOpNames[autoTrigs[i]] =
    AutoOpNames['arc'+autoTrigs[i]] =
    AutoOpNames[autoTrigs[i]+'h'] =
    AutoOpNames['ar'+autoTrigs[i]+'h'] =
    AutoOpNames['arc'+autoTrigs[i]+'h'] = 1;
  }

  // compat with some of the nonstandard LaTeX exported by MathQuill
  // before #247. None of these are real LaTeX commands so, seems safe
  var moreNonstandardOps = 'gcf hcf lcm proj span'.split(' ');
  for (var i = 0; i < moreNonstandardOps.length; i += 1) {
    AutoOpNames[moreNonstandardOps[i]] = 1;
  }
}());
optionProcessors.autoOperatorNames = function(cmds) {
  if (!/^[a-z]+(?: [a-z]+)*$/i.test(cmds)) {
    throw '"'+cmds+'" not a space-delimited list of only letters';
  }
  var list = cmds.split(' '), dict = {}, maxLength = 0;
  for (var i = 0; i < list.length; i += 1) {
    var cmd = list[i];
    if (cmd.length < 2) {
      throw '"'+cmd+'" not minimum length of 2';
    }
    dict[cmd] = 1;
    maxLength = max(maxLength, cmd.length);
  }
  dict._maxLength = maxLength;
  return dict;
};
var OperatorName = P(Symbol, function(_, super_) {
  _.init = function(fn) { this.ctrlSeq = fn; };
  _.createLeftOf = function(cursor) {
    var fn = this.ctrlSeq;
    for (var i = 0; i < fn.length; i += 1) {
      Letter(fn.charAt(i)).createLeftOf(cursor);
    }
  };
  _.parser = function() {
    var fn = this.ctrlSeq;
    var block = MathBlock();
    for (var i = 0; i < fn.length; i += 1) {
      Letter(fn.charAt(i)).adopt(block, block.ends[R], 0);
    }
    return Parser.succeed(block.children());
  };
});
for (var fn in AutoOpNames) if (AutoOpNames.hasOwnProperty(fn)) {
  LatexCmds[fn] = OperatorName;
}
LatexCmds.operatorname = P(MathCommand, function(_) {
  _.createLeftOf = noop;
  _.numBlocks = function() { return 1; };
  _.parser = function() {
    return latexMathParser.block.map(function(b) { return b.children(); });
  };
});

LatexCmds.f = P(Letter, function(_, super_) {
  _.init = function() {
    Symbol.p.init.call(this, this.letter = 'f', '<var class="mq-f">f</var>');
  };
  _.italicize = function(bool) {
    this.jQ.html('f').toggleClass('mq-f', bool);
    return super_.italicize.apply(this, arguments);
  };
});

// VanillaSymbol's
LatexCmds[' '] = LatexCmds.space = bind(VanillaSymbol, '\\ ', '&nbsp;');

LatexCmds["'"] = LatexCmds.prime = bind(VanillaSymbol, "'", '&prime;');

LatexCmds.backslash = bind(VanillaSymbol,'\\backslash ','\\');
if (!CharCmds['\\']) CharCmds['\\'] = LatexCmds.backslash;

LatexCmds.$ = bind(VanillaSymbol, '\\$', '$');

// does not use Symbola font
var NonSymbolaSymbol = P(Symbol, function(_, super_) {
  _.init = function(ch, html) {
    super_.init.call(this, ch, '<span class="mq-nonSymbola">'+(html || ch)+'</span>');
  };
});

LatexCmds['@'] = NonSymbolaSymbol;
LatexCmds['&'] = bind(NonSymbolaSymbol, '\\&', '&amp;');
LatexCmds['%'] = bind(NonSymbolaSymbol, '\\%', '%');

//the following are all Greek to me, but this helped a lot: http://www.ams.org/STIX/ion/stixsig03.html

//lowercase Greek letter variables
LatexCmds.alpha =
LatexCmds.beta =
LatexCmds.gamma =
LatexCmds.delta =
LatexCmds.zeta =
LatexCmds.eta =
LatexCmds.theta =
LatexCmds.iota =
LatexCmds.kappa =
LatexCmds.mu =
LatexCmds.nu =
LatexCmds.xi =
LatexCmds.rho =
LatexCmds.sigma =
LatexCmds.tau =
LatexCmds.chi =
LatexCmds.psi =
LatexCmds.omega = P(Variable, function(_, super_) {
  _.init = function(latex) {
    super_.init.call(this,'\\'+latex+' ','&'+latex+';');
  };
});

//why can't anybody FUCKING agree on these
LatexCmds.phi = //W3C or Unicode?
  bind(Variable,'\\phi ','&#981;');

LatexCmds.phiv = //Elsevier and 9573-13
LatexCmds.varphi = //AMS and LaTeX
  bind(Variable,'\\varphi ','&phi;');

LatexCmds.epsilon = //W3C or Unicode?
  bind(Variable,'\\epsilon ','&#1013;');

LatexCmds.epsiv = //Elsevier and 9573-13
LatexCmds.varepsilon = //AMS and LaTeX
  bind(Variable,'\\varepsilon ','&epsilon;');

LatexCmds.piv = //W3C/Unicode and Elsevier and 9573-13
LatexCmds.varpi = //AMS and LaTeX
  bind(Variable,'\\varpi ','&piv;');

LatexCmds.sigmaf = //W3C/Unicode
LatexCmds.sigmav = //Elsevier
LatexCmds.varsigma = //LaTeX
  bind(Variable,'\\varsigma ','&sigmaf;');

LatexCmds.thetav = //Elsevier and 9573-13
LatexCmds.vartheta = //AMS and LaTeX
LatexCmds.thetasym = //W3C/Unicode
  bind(Variable,'\\vartheta ','&thetasym;');

LatexCmds.upsilon = //AMS and LaTeX and W3C/Unicode
LatexCmds.upsi = //Elsevier and 9573-13
  bind(Variable,'\\upsilon ','&upsilon;');

//these aren't even mentioned in the HTML character entity references
LatexCmds.gammad = //Elsevier
LatexCmds.Gammad = //9573-13 -- WTF, right? I dunno if this was a typo in the reference (see above)
LatexCmds.digamma = //LaTeX
  bind(Variable,'\\digamma ','&#989;');

LatexCmds.kappav = //Elsevier
LatexCmds.varkappa = //AMS and LaTeX
  bind(Variable,'\\varkappa ','&#1008;');

LatexCmds.rhov = //Elsevier and 9573-13
LatexCmds.varrho = //AMS and LaTeX
  bind(Variable,'\\varrho ','&#1009;');

//Greek constants, look best in non-italicized Times New Roman
LatexCmds.pi = LatexCmds['π'] = bind(NonSymbolaSymbol,'\\pi ','&pi;');
LatexCmds.lambda = bind(NonSymbolaSymbol,'\\lambda ','&lambda;');

//uppercase greek letters

LatexCmds.Upsilon = //LaTeX
LatexCmds.Upsi = //Elsevier and 9573-13
LatexCmds.upsih = //W3C/Unicode "upsilon with hook"
LatexCmds.Upsih = //'cos it makes sense to me
  bind(Symbol,'\\Upsilon ','<var style="font-family: serif">&upsih;</var>'); //Symbola's 'upsilon with a hook' is a capital Y without hooks :(

//other symbols with the same LaTeX command and HTML character entity reference
LatexCmds.Gamma =
LatexCmds.Delta =
LatexCmds.Theta =
LatexCmds.Lambda =
LatexCmds.Xi =
LatexCmds.Pi =
LatexCmds.Sigma =
LatexCmds.Phi =
LatexCmds.Psi =
LatexCmds.Omega =
LatexCmds.forall = P(VanillaSymbol, function(_, super_) {
  _.init = function(latex) {
    super_.init.call(this,'\\'+latex+' ','&'+latex+';');
  };
});

// symbols that aren't a single MathCommand, but are instead a whole
// Fragment. Creates the Fragment from a LaTeX string
var LatexFragment = P(MathCommand, function(_) {
  _.init = function(latex) { this.latex = latex; };
  _.createLeftOf = function(cursor) {
    var block = latexMathParser.parse(this.latex);
    block.children().adopt(cursor.parent, cursor[L], cursor[R]);
    cursor[L] = block.ends[R];
    block.jQize().insertBefore(cursor.jQ);
    block.finalizeInsert(cursor.options, cursor);
    if (block.ends[R][R].siblingCreated) block.ends[R][R].siblingCreated(cursor.options, L);
    if (block.ends[L][L].siblingCreated) block.ends[L][L].siblingCreated(cursor.options, R);
    cursor.parent.bubble('reflow');
  };
  _.parser = function() {
    var frag = latexMathParser.parse(this.latex).children();
    return Parser.succeed(frag);
  };
});

// for what seems to me like [stupid reasons][1], Unicode provides
// subscripted and superscripted versions of all ten Arabic numerals,
// as well as [so-called "vulgar fractions"][2].
// Nobody really cares about most of them, but some of them actually
// predate Unicode, dating back to [ISO-8859-1][3], apparently also
// known as "Latin-1", which among other things [Windows-1252][4]
// largely coincides with, so Microsoft Word sometimes inserts them
// and they get copy-pasted into MathQuill.
//
// (Irrelevant but funny story: though not a superset of Latin-1 aka
// ISO-8859-1, Windows-1252 **is** a strict superset of the "closely
// related but distinct"[3] "ISO 8859-1" -- see the lack of a dash
// after "ISO"? Completely different character set, like elephants vs
// elephant seals, or "Zombies" vs "Zombie Redneck Torture Family".
// What kind of idiot would get them confused.
// People in fact got them confused so much, it was so common to
// mislabel Windows-1252 text as ISO-8859-1, that most modern web
// browsers and email clients treat the MIME charset of ISO-8859-1
// as actually Windows-1252, behavior now standard in the HTML5 spec.)
//
// [1]: http://en.wikipedia.org/wiki/Unicode_subscripts_andsuper_scripts
// [2]: http://en.wikipedia.org/wiki/Number_Forms
// [3]: http://en.wikipedia.org/wiki/ISO/IEC_8859-1
// [4]: http://en.wikipedia.org/wiki/Windows-1252
LatexCmds['¹'] = bind(LatexFragment, '^1');
LatexCmds['²'] = bind(LatexFragment, '^2');
LatexCmds['³'] = bind(LatexFragment, '^3');
LatexCmds['¼'] = bind(LatexFragment, '\\frac14');
LatexCmds['½'] = bind(LatexFragment, '\\frac12');
LatexCmds['¾'] = bind(LatexFragment, '\\frac34');

var PlusMinus = P(BinaryOperator, function(_) {
  _.init = VanillaSymbol.prototype.init;

  _.contactWeld = _.siblingCreated = _.siblingDeleted = function(opts, dir) {
    if (dir === R) return; // ignore if sibling only changed on the right
    this.jQ[0].className =
      (!this[L] || this[L] instanceof BinaryOperator ? '' : 'mq-binary-operator');
    return this;
  };
});

LatexCmds['+'] = bind(PlusMinus, '+', '+');
//yes, these are different dashes, I think one is an en dash and the other is a hyphen
LatexCmds['–'] = LatexCmds['-'] = bind(PlusMinus, '-', '&minus;');
LatexCmds['±'] = LatexCmds.pm = LatexCmds.plusmn = LatexCmds.plusminus =
  bind(PlusMinus,'\\pm ','&plusmn;');
LatexCmds.mp = LatexCmds.mnplus = LatexCmds.minusplus =
  bind(PlusMinus,'\\mp ','&#8723;');

CharCmds['*'] = LatexCmds.sdot = LatexCmds.cdot =
  bind(BinaryOperator, '\\cdot ', '&middot;', '*');
//semantically should be &sdot;, but &middot; looks better

var Inequality = P(BinaryOperator, function(_, super_) {
  _.init = function(data, strict) {
    this.data = data;
    this.strict = strict;
    var strictness = (strict ? 'Strict' : '');
    super_.init.call(this, data['ctrlSeq'+strictness], data['html'+strictness],
                     data['text'+strictness]);
  };
  _.swap = function(strict) {
    this.strict = strict;
    var strictness = (strict ? 'Strict' : '');
    this.ctrlSeq = this.data['ctrlSeq'+strictness];
    this.jQ.html(this.data['html'+strictness]);
    this.textTemplate = [ this.data['text'+strictness] ];
  };
  _.deleteTowards = function(dir, cursor) {
    if (dir === L && !this.strict) {
      this.swap(true);
      this.bubble('reflow');
      return;
    }
    super_.deleteTowards.apply(this, arguments);
  };
});

var less = { ctrlSeq: '\\le ', html: '&le;', text: '≤',
             ctrlSeqStrict: '<', htmlStrict: '&lt;', textStrict: '<' };
var greater = { ctrlSeq: '\\ge ', html: '&ge;', text: '≥',
                ctrlSeqStrict: '>', htmlStrict: '&gt;', textStrict: '>' };

LatexCmds['<'] = LatexCmds.lt = bind(Inequality, less, true);
LatexCmds['>'] = LatexCmds.gt = bind(Inequality, greater, true);
LatexCmds['≤'] = LatexCmds.le = LatexCmds.leq = bind(Inequality, less, false);
LatexCmds['≥'] = LatexCmds.ge = LatexCmds.geq = bind(Inequality, greater, false);

var Equality = P(BinaryOperator, function(_, super_) {
  _.init = function() {
    super_.init.call(this, '=', '=');
  };
  _.createLeftOf = function(cursor) {
    if (cursor[L] instanceof Inequality && cursor[L].strict) {
      cursor[L].swap(false);
      cursor[L].bubble('reflow');
      return;
    }
    super_.createLeftOf.apply(this, arguments);
  };
});
LatexCmds['='] = Equality;

LatexCmds['×'] = LatexCmds.times = bind(BinaryOperator, '\\times ', '&times;', '[x]');

LatexCmds['÷'] = LatexCmds.div = LatexCmds.divide = LatexCmds.divides =
  bind(BinaryOperator,'\\div ','&divide;', '[/]');

CharCmds['~'] = LatexCmds.sim = bind(BinaryOperator, '\\sim ', '~', '~');
/***************************
 * Commands and Operators.
 **************************/

var scale, // = function(jQ, x, y) { ... }
//will use a CSS 2D transform to scale the jQuery-wrapped HTML elements,
//or the filter matrix transform fallback for IE 5.5-8, or gracefully degrade to
//increasing the fontSize to match the vertical Y scaling factor.

//ideas from http://github.com/louisremi/jquery.transform.js
//see also http://msdn.microsoft.com/en-us/library/ms533014(v=vs.85).aspx

  forceIERedraw = noop,
  div = document.createElement('div'),
  div_style = div.style,
  transformPropNames = {
    transform:1,
    WebkitTransform:1,
    MozTransform:1,
    OTransform:1,
    msTransform:1
  },
  transformPropName;

for (var prop in transformPropNames) {
  if (prop in div_style) {
    transformPropName = prop;
    break;
  }
}

if (transformPropName) {
  scale = function(jQ, x, y) {
    jQ.css(transformPropName, 'scale('+x+','+y+')');
  };
}
else if ('filter' in div_style) { //IE 6, 7, & 8 fallback, see https://github.com/laughinghan/mathquill/wiki/Transforms
  forceIERedraw = function(el){ el.className = el.className; };
  scale = function(jQ, x, y) { //NOTE: assumes y > x
    x /= (1+(y-1)/2);
    jQ.css('fontSize', y + 'em');
    if (!jQ.hasClass('mq-matrixed-container')) {
      jQ.addClass('mq-matrixed-container')
      .wrapInner('<span class="mq-matrixed"></span>');
    }
    var innerjQ = jQ.children()
    .css('filter', 'progid:DXImageTransform.Microsoft'
        + '.Matrix(M11=' + x + ",SizingMethod='auto expand')"
    );
    function calculateMarginRight() {
      jQ.css('marginRight', (innerjQ.width()-1)*(x-1)/x + 'px');
    }
    calculateMarginRight();
    var intervalId = setInterval(calculateMarginRight);
    $(window).load(function() {
      clearTimeout(intervalId);
      calculateMarginRight();
    });
  };
}
else {
  scale = function(jQ, x, y) {
    jQ.css('fontSize', y + 'em');
  };
}

var Style = P(MathCommand, function(_, super_) {
  _.init = function(ctrlSeq, tagName, attrs) {
    super_.init.call(this, ctrlSeq, '<'+tagName+' '+attrs+'>&0</'+tagName+'>');
  };
});

//fonts
LatexCmds.mathrm = bind(Style, '\\mathrm', 'span', 'class="mq-roman mq-font"');
LatexCmds.mathit = bind(Style, '\\mathit', 'i', 'class="mq-font"');
LatexCmds.mathbf = bind(Style, '\\mathbf', 'b', 'class="mq-font"');
LatexCmds.mathsf = bind(Style, '\\mathsf', 'span', 'class="mq-sans-serif mq-font"');
LatexCmds.mathtt = bind(Style, '\\mathtt', 'span', 'class="mq-monospace mq-font"');
//text-decoration
LatexCmds.underline = bind(Style, '\\underline', 'span', 'class="mq-non-leaf mq-underline"');
LatexCmds.overline = LatexCmds.bar = bind(Style, '\\overline', 'span', 'class="mq-non-leaf mq-overline"');
LatexCmds.overrightarrow = bind(Style, '\\overrightarrow', 'span', 'class="mq-non-leaf mq-overarrow mq-arrow-right"');
LatexCmds.overleftarrow = bind(Style, '\\overleftarrow', 'span', 'class="mq-non-leaf mq-overarrow mq-arrow-left"');

// `\textcolor{color}{math}` will apply a color to the given math content, where
// `color` is any valid CSS Color Value (see [SitePoint docs][] (recommended),
// [Mozilla docs][], or [W3C spec][]).
//
// [SitePoint docs]: http://reference.sitepoint.com/css/colorvalues
// [Mozilla docs]: https://developer.mozilla.org/en-US/docs/CSS/color_value#Values
// [W3C spec]: http://dev.w3.org/csswg/css3-color/#colorunits
var TextColor = LatexCmds.textcolor = P(MathCommand, function(_, super_) {
  _.setColor = function(color) {
    this.color = color;
    this.htmlTemplate =
      '<span class="mq-textcolor" style="color:' + color + '">&0</span>';
  };
  _.latex = function() {
    return '\\textcolor{' + this.color + '}{' + this.blocks[0].latex() + '}';
  };
  _.parser = function() {
    var self = this;
    var optWhitespace = Parser.optWhitespace;
    var string = Parser.string;
    var regex = Parser.regex;

    return optWhitespace
      .then(string('{'))
      .then(regex(/^[#\w\s.,()%-]*/))
      .skip(string('}'))
      .then(function(color) {
        self.setColor(color);
        return super_.parser.call(self);
      })
    ;
  };
});

// Very similar to the \textcolor command, but will add the given CSS class.
// Usage: \class{classname}{math}
// Note regex that whitelists valid CSS classname characters:
// https://github.com/mathquill/mathquill/pull/191#discussion_r4327442
var Class = LatexCmds['class'] = P(MathCommand, function(_, super_) {
  _.parser = function() {
    var self = this, string = Parser.string, regex = Parser.regex;
    return Parser.optWhitespace
      .then(string('{'))
      .then(regex(/^[-\w\s\\\xA0-\xFF]*/))
      .skip(string('}'))
      .then(function(cls) {
        self.htmlTemplate = '<span class="mq-class '+cls+'">&0</span>';
        return super_.parser.call(self);
      })
    ;
  };
});

var SupSub = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '_{...}^{...}';
  _.createLeftOf = function(cursor) {
    if (!cursor[L] && cursor.options.supSubsRequireOperand) return;
    return super_.createLeftOf.apply(this, arguments);
  };
  _.contactWeld = function(cursor) {
    // Look on either side for a SupSub, if one is found compare my
    // .sub, .sup with its .sub, .sup. If I have one that it doesn't,
    // then call .addBlock() on it with my block; if I have one that
    // it also has, then insert my block's children into its block,
    // unless my block has none, in which case insert the cursor into
    // its block (and not mine, I'm about to remove myself) in the case
    // I was just typed.
    // TODO: simplify

    // equiv. to [L, R].forEach(function(dir) { ... });
    for (var dir = L; dir; dir = (dir === L ? R : false)) {
      if (this[dir] instanceof SupSub) {
        // equiv. to 'sub sup'.split(' ').forEach(function(supsub) { ... });
        for (var supsub = 'sub'; supsub; supsub = (supsub === 'sub' ? 'sup' : false)) {
          var src = this[supsub], dest = this[dir][supsub];
          if (!src) continue;
          if (!dest) this[dir].addBlock(src.disown());
          else if (!src.isEmpty()) { // ins src children at -dir end of dest
            src.jQ.children().insAtDirEnd(-dir, dest.jQ);
            var children = src.children().disown();
            var pt = Point(dest, children.ends[R], dest.ends[L]);
            if (dir === L) children.adopt(dest, dest.ends[R], 0);
            else children.adopt(dest, 0, dest.ends[L]);
          }
          else var pt = Point(dest, 0, dest.ends[L]);
          this.placeCursor = (function(dest, src) { // TODO: don't monkey-patch
            return function(cursor) { cursor.insAtDirEnd(-dir, dest || src); };
          }(dest, src));
        }
        this.remove();
        if (cursor && cursor[L] === this) {
          if (dir === R && pt) {
            pt[L] ? cursor.insRightOf(pt[L]) : cursor.insAtLeftEnd(pt.parent);
          }
          else cursor.insRightOf(this[dir]);
        }
        break;
      }
    }
    this.respace();
  };
  Options.p.charsThatBreakOutOfSupSub = '';
  _.finalizeTree = function() {
    this.ends[L].write = function(cursor, ch) {
      if (cursor.options.autoSubscriptNumerals && this === this.parent.sub) {
        if (ch === '_') return;
        var cmd = this.chToCmd(ch);
        if (cmd instanceof Symbol) cursor.deleteSelection();
        else cursor.clearSelection().insRightOf(this.parent);
        return cmd.createLeftOf(cursor.show());
      }
      if (cursor[L] && !cursor[R] && !cursor.selection
          && cursor.options.charsThatBreakOutOfSupSub.indexOf(ch) > -1) {
        cursor.insRightOf(this.parent);
      }
      MathBlock.p.write.apply(this, arguments);
    };
  };
  _.moveTowards = function(dir, cursor, updown) {
    if (cursor.options.autoSubscriptNumerals && !this.sup) {
      cursor.insDirOf(dir, this);
    }
    else super_.moveTowards.apply(this, arguments);
  };
  _.deleteTowards = function(dir, cursor) {
    if (cursor.options.autoSubscriptNumerals && this.sub) {
      var cmd = this.sub.ends[-dir];
      if (cmd instanceof Symbol) cmd.remove();
      else if (cmd) cmd.deleteTowards(dir, cursor.insAtDirEnd(-dir, this.sub));

      // TODO: factor out a .removeBlock() or something
      if (this.sub.isEmpty()) {
        this.sub.deleteOutOf(L, cursor.insAtLeftEnd(this.sub));
        if (this.sup) cursor.insDirOf(-dir, this);
        // Note `-dir` because in e.g. x_1^2| want backspacing (leftward)
        // to delete the 1 but to end up rightward of x^2; with non-negated
        // `dir` (try it), the cursor appears to have gone "through" the ^2.
      }
    }
    else super_.deleteTowards.apply(this, arguments);
  };
  _.latex = function() {
    function latex(prefix, block) {
      var l = block && block.latex();
      return block ? prefix + (l.length === 1 ? l : '{' + (l || ' ') + '}') : '';
    }
    return latex('_', this.sub) + latex('^', this.sup);
  };
  _.respace = _.siblingCreated = _.siblingDeleted = function(opts, dir) {
    if (dir === R) return; // ignore if sibling only changed on the right
    this.jQ.toggleClass('mq-limit', this[L].ctrlSeq === '\\int ');
  };
  _.addBlock = function(block) {
    if (this.supsub === 'sub') {
      this.sup = this.upInto = this.sub.upOutOf = block;
      block.adopt(this, this.sub, 0).downOutOf = this.sub;
      block.jQ = $('<span class="mq-sup"/>').append(block.jQ.children())
        .attr(mqBlockId, block.id).prependTo(this.jQ);
    }
    else {
      this.sub = this.downInto = this.sup.downOutOf = block;
      block.adopt(this, 0, this.sup).upOutOf = this.sup;
      block.jQ = $('<span class="mq-sub"></span>').append(block.jQ.children())
        .attr(mqBlockId, block.id).appendTo(this.jQ.removeClass('mq-sup-only'));
      this.jQ.append('<span style="display:inline-block;width:0">&#8203;</span>');
    }
    // like 'sub sup'.split(' ').forEach(function(supsub) { ... });
    for (var i = 0; i < 2; i += 1) (function(cmd, supsub, oppositeSupsub, updown) {
      cmd[supsub].deleteOutOf = function(dir, cursor) {
        cursor.insDirOf((this[dir] ? -dir : dir), this.parent);
        if (!this.isEmpty()) {
          var end = this.ends[dir];
          this.children().disown()
            .withDirAdopt(dir, cursor.parent, cursor[dir], cursor[-dir])
            .jQ.insDirOf(-dir, cursor.jQ);
          cursor[-dir] = end;
        }
        cmd.supsub = oppositeSupsub;
        delete cmd[supsub];
        delete cmd[updown+'Into'];
        cmd[oppositeSupsub][updown+'OutOf'] = insLeftOfMeUnlessAtEnd;
        delete cmd[oppositeSupsub].deleteOutOf;
        if (supsub === 'sub') $(cmd.jQ.addClass('mq-sup-only')[0].lastChild).remove();
        this.remove();
      };
    }(this, 'sub sup'.split(' ')[i], 'sup sub'.split(' ')[i], 'down up'.split(' ')[i]));
  };
});

function insLeftOfMeUnlessAtEnd(cursor) {
  // cursor.insLeftOf(cmd), unless cursor at the end of block, and every
  // ancestor cmd is at the end of every ancestor block
  var cmd = this.parent, ancestorCmd = cursor;
  do {
    if (ancestorCmd[R]) return cursor.insLeftOf(cmd);
    ancestorCmd = ancestorCmd.parent.parent;
  } while (ancestorCmd !== cmd);
  cursor.insRightOf(cmd);
}

LatexCmds.subscript =
LatexCmds._ = P(SupSub, function(_, super_) {
  _.supsub = 'sub';
  _.htmlTemplate =
      '<span class="mq-supsub mq-non-leaf">'
    +   '<span class="mq-sub">&0</span>'
    +   '<span style="display:inline-block;width:0">&#8203;</span>'
    + '</span>'
  ;
  _.textTemplate = [ '_' ];
  _.finalizeTree = function() {
    this.downInto = this.sub = this.ends[L];
    this.sub.upOutOf = insLeftOfMeUnlessAtEnd;
    super_.finalizeTree.call(this);
  };
});

LatexCmds.superscript =
LatexCmds.supscript =
LatexCmds['^'] = P(SupSub, function(_, super_) {
  _.supsub = 'sup';
  _.htmlTemplate =
      '<span class="mq-supsub mq-non-leaf mq-sup-only">'
    +   '<span class="mq-sup">&0</span>'
    + '</span>'
  ;
  _.textTemplate = [ '^' ];
  _.finalizeTree = function() {
    this.upInto = this.sup = this.ends[R];
    this.sup.downOutOf = insLeftOfMeUnlessAtEnd;
    super_.finalizeTree.call(this);
  };
});

var SummationNotation = P(MathCommand, function(_, super_) {
  _.init = function(ch, html) {
    var htmlTemplate =
      '<span class="mq-large-operator mq-non-leaf">'
    +   '<span class="mq-to"><span>&1</span></span>'
    +   '<big>'+html+'</big>'
    +   '<span class="mq-from"><span>&0</span></span>'
    + '</span>'
    ;
    Symbol.prototype.init.call(this, ch, htmlTemplate);
  };
  _.createLeftOf = function(cursor) {
    super_.createLeftOf.apply(this, arguments);
    if (cursor.options.sumStartsWithNEquals) {
      Letter('n').createLeftOf(cursor);
      Equality().createLeftOf(cursor);
    }
  };
  _.latex = function() {
    function simplify(latex) {
      return latex.length === 1 ? latex : '{' + (latex || ' ') + '}';
    }
    return this.ctrlSeq + '_' + simplify(this.ends[L].latex()) +
      '^' + simplify(this.ends[R].latex());
  };
  _.parser = function() {
    var string = Parser.string;
    var optWhitespace = Parser.optWhitespace;
    var succeed = Parser.succeed;
    var block = latexMathParser.block;

    var self = this;
    var blocks = self.blocks = [ MathBlock(), MathBlock() ];
    for (var i = 0; i < blocks.length; i += 1) {
      blocks[i].adopt(self, self.ends[R], 0);
    }

    return optWhitespace.then(string('_').or(string('^'))).then(function(supOrSub) {
      var child = blocks[supOrSub === '_' ? 0 : 1];
      return block.then(function(block) {
        block.children().adopt(child, child.ends[R], 0);
        return succeed(self);
      });
    }).many().result(self);
  };
  _.finalizeTree = function() {
    this.downInto = this.ends[L];
    this.upInto = this.ends[R];
    this.ends[L].upOutOf = this.ends[R];
    this.ends[R].downOutOf = this.ends[L];
  };
});

LatexCmds['∑'] =
LatexCmds.sum =
LatexCmds.summation = bind(SummationNotation,'\\sum ','&sum;');

LatexCmds['∏'] =
LatexCmds.prod =
LatexCmds.product = bind(SummationNotation,'\\prod ','&prod;');

LatexCmds.coprod =
LatexCmds.coproduct = bind(SummationNotation,'\\coprod ','&#8720;');

var Fraction =
LatexCmds.frac =
LatexCmds.dfrac =
LatexCmds.cfrac =
LatexCmds.fraction = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '\\frac';
  _.htmlTemplate =
      '<span class="mq-fraction mq-non-leaf">'
    +   '<span class="mq-numerator">&0</span>'
    +   '<span class="mq-denominator">&1</span>'
    +   '<span style="display:inline-block;width:0">&#8203;</span>'
    + '</span>'
  ;
  _.textTemplate = ['(', ')/(', ')'];
  _.finalizeTree = function() {
    this.upInto = this.ends[R].upOutOf = this.ends[L];
    this.downInto = this.ends[L].downOutOf = this.ends[R];
  };
});

var LiveFraction =
LatexCmds.over =
CharCmds['/'] = P(Fraction, function(_, super_) {
  _.createLeftOf = function(cursor) {
    if (!this.replacedFragment) {
      var leftward = cursor[L];
      while (leftward &&
        !(
          leftward instanceof BinaryOperator ||
          leftward instanceof (LatexCmds.text || noop) ||
          leftward instanceof SummationNotation ||
          leftward.ctrlSeq === '\\ ' ||
          /^[,;:]$/.test(leftward.ctrlSeq)
        ) //lookbehind for operator
      ) leftward = leftward[L];

      if (leftward instanceof SummationNotation && leftward[R] instanceof SupSub) {
        leftward = leftward[R];
        if (leftward[R] instanceof SupSub && leftward[R].ctrlSeq != leftward.ctrlSeq)
          leftward = leftward[R];
      }

      if (leftward !== cursor[L]) {
        this.replaces(Fragment(leftward[R] || cursor.parent.ends[L], cursor[L]));
        cursor[L] = leftward;
      }
    }
    super_.createLeftOf.call(this, cursor);
  };
});

var SquareRoot =
LatexCmds.sqrt =
LatexCmds['√'] = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '\\sqrt';
  _.htmlTemplate =
      '<span class="mq-non-leaf">'
    +   '<span class="mq-scaled mq-sqrt-prefix">&radic;</span>'
    +   '<span class="mq-non-leaf mq-sqrt-stem">&0</span>'
    + '</span>'
  ;
  _.textTemplate = ['sqrt(', ')'];
  _.parser = function() {
    return latexMathParser.optBlock.then(function(optBlock) {
      return latexMathParser.block.map(function(block) {
        var nthroot = NthRoot();
        nthroot.blocks = [ optBlock, block ];
        optBlock.adopt(nthroot, 0, 0);
        block.adopt(nthroot, optBlock, 0);
        return nthroot;
      });
    }).or(super_.parser.call(this));
  };
  _.reflow = function() {
    var block = this.ends[R].jQ;
    scale(block.prev(), 1, block.innerHeight()/+block.css('fontSize').slice(0,-2) - .1);
  };
});

var Vec = LatexCmds.vec = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '\\vec';
  _.htmlTemplate =
      '<span class="mq-non-leaf">'
    +   '<span class="mq-vector-prefix">&rarr;</span>'
    +   '<span class="mq-vector-stem">&0</span>'
    + '</span>'
  ;
  _.textTemplate = ['vec(', ')'];
});

var NthRoot =
LatexCmds.nthroot = P(SquareRoot, function(_, super_) {
  _.htmlTemplate =
      '<sup class="mq-nthroot mq-non-leaf">&0</sup>'
    + '<span class="mq-scaled">'
    +   '<span class="mq-sqrt-prefix mq-scaled">&radic;</span>'
    +   '<span class="mq-sqrt-stem mq-non-leaf">&1</span>'
    + '</span>'
  ;
  _.textTemplate = ['sqrt[', '](', ')'];
  _.latex = function() {
    return '\\sqrt['+this.ends[L].latex()+']{'+this.ends[R].latex()+'}';
  };
});

function DelimsMixin(_, super_) {
  _.jQadd = function() {
    super_.jQadd.apply(this, arguments);
    this.delimjQs = this.jQ.children(':first').add(this.jQ.children(':last'));
    this.contentjQ = this.jQ.children(':eq(1)');
  };
  _.reflow = function() {
    var height = this.contentjQ.outerHeight()
                 / parseFloat(this.contentjQ.css('fontSize'));
    scale(this.delimjQs, min(1 + .2*(height - 1), 1.2), 1.2*height);
  };
}

// Round/Square/Curly/Angle Brackets (aka Parens/Brackets/Braces)
//   first typed as one-sided bracket with matching "ghost" bracket at
//   far end of current block, until you type an opposing one
var Bracket = P(P(MathCommand, DelimsMixin), function(_, super_) {
  _.init = function(side, open, close, ctrlSeq, end) {
    super_.init.call(this, '\\left'+ctrlSeq, undefined, [open, close]);
    this.side = side;
    this.sides = {};
    this.sides[L] = { ch: open, ctrlSeq: ctrlSeq };
    this.sides[R] = { ch: close, ctrlSeq: end };
  };
  _.numBlocks = function() { return 1; };
  _.html = function() { // wait until now so that .side may
    this.htmlTemplate = // be set by createLeftOf or parser
        '<span class="mq-non-leaf">'
      +   '<span class="mq-scaled mq-paren'+(this.side === R ? ' mq-ghost' : '')+'">'
      +     this.sides[L].ch
      +   '</span>'
      +   '<span class="mq-non-leaf">&0</span>'
      +   '<span class="mq-scaled mq-paren'+(this.side === L ? ' mq-ghost' : '')+'">'
      +     this.sides[R].ch
      +   '</span>'
      + '</span>'
    ;
    return super_.html.call(this);
  };
  _.latex = function() {
    return '\\left'+this.sides[L].ctrlSeq+this.ends[L].latex()+'\\right'+this.sides[R].ctrlSeq;
  };
  _.oppBrack = function(opts, node, expectedSide) {
    // return node iff it's a 1-sided bracket of expected side (if any, may be
    // undefined), and of opposite side from me if I'm not a pipe
    return node instanceof Bracket && node.side && node.side !== -expectedSide
      && (this.sides[this.side].ch === '|' || node.side === -this.side)
      && (!opts.restrictMismatchedBrackets
        || OPP_BRACKS[this.sides[this.side].ch] === node.sides[node.side].ch
        || { '(': ']', '[': ')' }[this.sides[L].ch] === node.sides[R].ch) && node;
  };
  _.closeOpposing = function(brack) {
    brack.side = 0;
    brack.sides[this.side] = this.sides[this.side]; // copy over my info (may be
    brack.delimjQs.eq(this.side === L ? 0 : 1) // mismatched, like [a, b))
      .removeClass('mq-ghost').html(this.sides[this.side].ch);
  };
  _.createLeftOf = function(cursor) {
    if (!this.replacedFragment) { // unless wrapping seln in brackets,
        // check if next to or inside an opposing one-sided bracket
        // (must check both sides 'cos I might be a pipe)
      var opts = cursor.options;
      var brack = this.oppBrack(opts, cursor[L], L)
                  || this.oppBrack(opts, cursor[R], R)
                  || this.oppBrack(opts, cursor.parent.parent);
    }
    if (brack) {
      var side = this.side = -brack.side; // may be pipe with .side not yet set
      this.closeOpposing(brack);
      if (brack === cursor.parent.parent && cursor[side]) { // move the stuff between
        Fragment(cursor[side], cursor.parent.ends[side], -side) // me and ghost outside
          .disown().withDirAdopt(-side, brack.parent, brack, brack[side])
          .jQ.insDirOf(side, brack.jQ);
        brack.bubble('reflow');
      }
    }
    else {
      brack = this, side = brack.side;
      if (brack.replacedFragment) brack.side = 0; // wrapping seln, don't be one-sided
      else if (cursor[-side]) { // elsewise, auto-expand so ghost is at far end
        brack.replaces(Fragment(cursor[-side], cursor.parent.ends[-side], side));
        cursor[-side] = 0;
      }
      super_.createLeftOf.call(brack, cursor);
    }
    if (side === L) cursor.insAtLeftEnd(brack.ends[L]);
    else cursor.insRightOf(brack);
  };
  _.placeCursor = noop;
  _.unwrap = function() {
    this.ends[L].children().disown().adopt(this.parent, this, this[R])
      .jQ.insertAfter(this.jQ);
    this.remove();
  };
  _.deleteSide = function(side, outward, cursor) {
    var parent = this.parent, sib = this[side], farEnd = parent.ends[side];

    if (side === this.side) { // deleting non-ghost of one-sided bracket, unwrap
      this.unwrap();
      sib ? cursor.insDirOf(-side, sib) : cursor.insAtDirEnd(side, parent);
      return;
    }

    var opts = cursor.options, wasSolid = !this.side;
    this.side = -side;
    // if deleting like, outer close-brace of [(1+2)+3} where inner open-paren
    if (this.oppBrack(opts, this.ends[L].ends[this.side], side)) { // is ghost,
      this.closeOpposing(this.ends[L].ends[this.side]); // then become [1+2)+3
      var origEnd = this.ends[L].ends[side];
      this.unwrap();
      if (origEnd.siblingCreated) origEnd.siblingCreated(cursor.options, side);
      sib ? cursor.insDirOf(-side, sib) : cursor.insAtDirEnd(side, parent);
    }
    else { // if deleting like, inner close-brace of ([1+2}+3) where outer
      if (this.oppBrack(opts, this.parent.parent, side)) { // open-paren is
        this.parent.parent.closeOpposing(this); // ghost, then become [1+2+3)
        this.parent.parent.unwrap();
      } // else if deleting outward from a solid pair, unwrap
      else if (outward && wasSolid) {
        this.unwrap();
        sib ? cursor.insDirOf(-side, sib) : cursor.insAtDirEnd(side, parent);
        return;
      }
      else { // else deleting just one of a pair of brackets, become one-sided
        this.sides[side] = { ch: OPP_BRACKS[this.sides[this.side].ch],
                             ctrlSeq: OPP_BRACKS[this.sides[this.side].ctrlSeq] };
        this.delimjQs.removeClass('mq-ghost')
          .eq(side === L ? 0 : 1).addClass('mq-ghost').html(this.sides[side].ch);
      }
      if (sib) { // auto-expand so ghost is at far end
        var origEnd = this.ends[L].ends[side];
        Fragment(sib, farEnd, -side).disown()
          .withDirAdopt(-side, this.ends[L], origEnd, 0)
          .jQ.insAtDirEnd(side, this.ends[L].jQ.removeClass('mq-empty'));
        if (origEnd.siblingCreated) origEnd.siblingCreated(cursor.options, side);
        cursor.insDirOf(-side, sib);
      } // didn't auto-expand, cursor goes just outside or just inside parens
      else (outward ? cursor.insDirOf(side, this)
                    : cursor.insAtDirEnd(side, this.ends[L]));
    }
  };
  _.deleteTowards = function(dir, cursor) {
    this.deleteSide(-dir, false, cursor);
  };
  _.finalizeTree = function() {
    this.ends[L].deleteOutOf = function(dir, cursor) {
      this.parent.deleteSide(dir, true, cursor);
    };
    // FIXME HACK: after initial creation/insertion, finalizeTree would only be
    // called if the paren is selected and replaced, e.g. by LiveFraction
    this.finalizeTree = this.intentionalBlur = function() {
      this.delimjQs.eq(this.side === L ? 1 : 0).removeClass('mq-ghost');
      this.side = 0;
    };
  };
  _.siblingCreated = function(opts, dir) { // if something typed between ghost and far
    if (dir === -this.side) this.finalizeTree(); // end of its block, solidify
  };
});

var OPP_BRACKS = {
  '(': ')',
  ')': '(',
  '[': ']',
  ']': '[',
  '{': '}',
  '}': '{',
  '\\{': '\\}',
  '\\}': '\\{',
  '&lang;': '&rang;',
  '&rang;': '&lang;',
  '\\langle ': '\\rangle ',
  '\\rangle ': '\\langle ',
  '|': '|'
};

function bindCharBracketPair(open, ctrlSeq) {
  var ctrlSeq = ctrlSeq || open, close = OPP_BRACKS[open], end = OPP_BRACKS[ctrlSeq];
  CharCmds[open] = bind(Bracket, L, open, close, ctrlSeq, end);
  CharCmds[close] = bind(Bracket, R, open, close, ctrlSeq, end);
}
bindCharBracketPair('(');
bindCharBracketPair('[');
bindCharBracketPair('{', '\\{');
LatexCmds.langle = bind(Bracket, L, '&lang;', '&rang;', '\\langle ', '\\rangle ');
LatexCmds.rangle = bind(Bracket, R, '&lang;', '&rang;', '\\langle ', '\\rangle ');
CharCmds['|'] = bind(Bracket, L, '|', '|', '|', '|');

LatexCmds.left = P(MathCommand, function(_) {
  _.parser = function() {
    var regex = Parser.regex;
    var string = Parser.string;
    var succeed = Parser.succeed;
    var optWhitespace = Parser.optWhitespace;

    return optWhitespace.then(regex(/^(?:[([|]|\\\{)/))
      .then(function(ctrlSeq) { // TODO: \langle, \rangle
        var open = (ctrlSeq.charAt(0) === '\\' ? ctrlSeq.slice(1) : ctrlSeq);
        return latexMathParser.then(function (block) {
          return string('\\right').skip(optWhitespace)
            .then(regex(/^(?:[\])|]|\\\})/)).map(function(end) {
              var close = (end.charAt(0) === '\\' ? end.slice(1) : end);
              var cmd = Bracket(0, open, close, ctrlSeq, end);
              cmd.blocks = [ block ];
              block.adopt(cmd, 0, 0);
              return cmd;
            })
          ;
        });
      })
    ;
  };
});

LatexCmds.right = P(MathCommand, function(_) {
  _.parser = function() {
    return Parser.fail('unmatched \\right');
  };
});

var Binomial =
LatexCmds.binom =
LatexCmds.binomial = P(P(MathCommand, DelimsMixin), function(_, super_) {
  _.ctrlSeq = '\\binom';
  _.htmlTemplate =
      '<span class="mq-non-leaf">'
    +   '<span class="mq-paren mq-scaled">(</span>'
    +   '<span class="mq-non-leaf">'
    +     '<span class="mq-array mq-non-leaf">'
    +       '<span>&0</span>'
    +       '<span>&1</span>'
    +     '</span>'
    +   '</span>'
    +   '<span class="mq-paren mq-scaled">)</span>'
    + '</span>'
  ;
  _.textTemplate = ['choose(',',',')'];
});

var Choose =
LatexCmds.choose = P(Binomial, function(_) {
  _.createLeftOf = LiveFraction.prototype.createLeftOf;
});

LatexCmds.editable = // backcompat with before cfd3620 on #233
LatexCmds.MathQuillMathField = P(MathCommand, function(_, super_) {
  _.ctrlSeq = '\\MathQuillMathField';
  _.htmlTemplate =
      '<span class="mq-editable-field">'
    +   '<span class="mq-root-block">&0</span>'
    + '</span>'
  ;
  _.parser = function() {
    var self = this,
      string = Parser.string, regex = Parser.regex, succeed = Parser.succeed;
    return string('[').then(regex(/^[a-z][a-z0-9]*/i)).skip(string(']'))
      .map(function(name) { self.name = name; }).or(succeed())
      .then(super_.parser.call(self));
  };
  _.finalizeTree = function() {
    var ctrlr = Controller(this.ends[L], this.jQ, Options());
    ctrlr.KIND_OF_MQ = 'MathField';
    ctrlr.editable = true;
    ctrlr.createTextarea();
    ctrlr.editablesTextareaEvents();
    ctrlr.cursor.insAtRightEnd(ctrlr.root);
    RootBlockMixin(ctrlr.root);
  };
  _.registerInnerField = function(innerFields, MathField) {
    innerFields.push(innerFields[this.name] = MathField(this.ends[L].controller));
  };
  _.latex = function(){ return this.ends[L].latex(); };
  _.text = function(){ return this.ends[L].text(); };
});

// Embed arbitrary things
// Probably the closest DOM analogue would be an iframe?
// From MathQuill's perspective, it's a Symbol, it can be
// anywhere and the cursor can go around it but never in it.
// Create by calling public API method .dropEmbedded(),
// or by calling the global public API method .registerEmbed()
// and rendering LaTeX like \embed{registeredName} (see test).
var Embed = LatexCmds.embed = P(Symbol, function(_, super_) {
  _.setOptions = function(options) {
    function noop () { return ""; }
    this.text = options.text || noop;
    this.htmlTemplate = options.htmlString || "";
    this.latex = options.latex || noop;
    return this;
  };
  _.parser = function() {
    var self = this;
      string = Parser.string, regex = Parser.regex, succeed = Parser.succeed;
    return string('{').then(regex(/^[a-z][a-z0-9]*/i)).skip(string('}'))
      .then(function(name) {
        // the chars allowed in the optional data block are arbitrary other than
        // excluding curly braces and square brackets (which'd be too confusing)
        return string('[').then(regex(/^[-\w\s]*/)).skip(string(']'))
          .or(succeed()).map(function(data) {
            return self.setOptions(EMBEDS[name](data));
          })
        ;
      })
    ;
  };
});
suite('SupSub', function() {
  var mq;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  function prayWellFormedPoint(pt) { prayWellFormed(pt.parent, pt[L], pt[R]); }

  var expecteds = [
    'x_{ab} x_{ba}, x_a^b x_a^b; x_{ab} x_{ba}, x_a^b x_a^b; x_a x_a, x_a^{} x_a^{}',
    'x_b^a x_b^a, x^{ab} x^{ba}; x_b^a x_b^a, x^{ab} x^{ba}; x_{}^a x_{}^a, x^a x^a'
  ];
  var expectedsAfterC = [
    'x_{abc} x_{bca}, x_a^{bc} x_a^{bc}; x_{ab}c x_{bca}, x_a^bc x_a^bc; x_ac x_{ca}, x_a^{}c x_a^{}c',
    'x_{bc}^a x_{bc}^a, x^{abc} x^{bca}; x_b^ac x_b^ac, x^{ab}c x^{bca}; x_{}^ac x_{}^ac, x^ac x^{ca}'
  ];
  'sub super'.split(' ').forEach(function(initSupsub, i) {
    var initialLatex = 'x_a x^a'.split(' ')[i];

    'typed, wrote, wrote empty'.split(', ').forEach(function(did, j) {
      var doTo = [
        function(mq, supsub) { mq.typedText(supsub).typedText('b'); },
        function(mq, supsub) { mq.write(supsub+'b'); },
        function(mq, supsub) { mq.write(supsub+'{}'); }
      ][j];

      'sub super'.split(' ').forEach(function(supsub, k) {
        var cmd = '_^'.split('')[k];

        'after before'.split(' ').forEach(function(side, l) {
          var moveToSide = [
            noop,
            function(mq) { mq.moveToLeftEnd().keystroke('Right'); }
          ][l];

          var expected = expecteds[i].split('; ')[j].split(', ')[k].split(' ')[l];
          var expectedAfterC = expectedsAfterC[i].split('; ')[j].split(', ')[k].split(' ')[l];

          test('initial '+initSupsub+'script then '+did+' '+supsub+'script '+side, function() {
            mq.latex(initialLatex);
            assert.equal(mq.latex(), initialLatex);

            moveToSide(mq);

            doTo(mq, cmd);
            assert.equal(mq.latex().replace(/ /g, ''), expected);

            prayWellFormedPoint(mq.__controller.cursor);

            mq.typedText('c');
            assert.equal(mq.latex().replace(/ /g, ''), expectedAfterC);
          });
        });
      });
    });
  });

  var expecteds = 'x_a^3 x_a^3, x_a^3 x_a^3; x^{a3} x^{3a}, x^{a3} x^{3a}';
  var expectedsAfterC = 'x_a^3c x_a^3c, x_a^3c x_a^3c; x^{a3}c x^{3ca}, x^{a3}c x^{3ca}';
  'sub super'.split(' ').forEach(function(initSupsub, i) {
    var initialLatex = 'x_a x^a'.split(' ')[i];

    'typed wrote'.split(' ').forEach(function(did, j) {
      var doTo = [
        function(mq) { mq.typedText('³'); },
        function(mq) { mq.write('³'); }
      ][j];

      'after before'.split(' ').forEach(function(side, k) {
        var moveToSide = [
          noop,
          function(mq) { mq.moveToLeftEnd().keystroke('Right'); }
        ][k];

        var expected = expecteds.split('; ')[i].split(', ')[j].split(' ')[k];
        var expectedAfterC = expectedsAfterC.split('; ')[i].split(', ')[j].split(' ')[k];

        test('initial '+initSupsub+'script then '+did+' \'³\' '+side, function() {
          mq.latex(initialLatex);
          assert.equal(mq.latex(), initialLatex);

          moveToSide(mq);

          doTo(mq);
          assert.equal(mq.latex().replace(/ /g, ''), expected);

          prayWellFormedPoint(mq.__controller.cursor);

          mq.typedText('c');
          assert.equal(mq.latex().replace(/ /g, ''), expectedAfterC);
        });
      });
    });
  });

  test('render LaTeX with 2 SupSub\'s in a row', function() {
    mq.latex('x_a_b');
    assert.equal(mq.latex(), 'x_{ab}');

    mq.latex('x_a_{}');
    assert.equal(mq.latex(), 'x_a');

    mq.latex('x_{}_a');
    assert.equal(mq.latex(), 'x_a');

    mq.latex('x^a^b');
    assert.equal(mq.latex(), 'x^{ab}');

    mq.latex('x^a^{}');
    assert.equal(mq.latex(), 'x^a');

    mq.latex('x^{}^a');
    assert.equal(mq.latex(), 'x^a');
  });

  test('render LaTeX with 3 alternating SupSub\'s in a row', function() {
    mq.latex('x_a^b_c');
    assert.equal(mq.latex(), 'x_{ac}^b');

    mq.latex('x^a_b^c');
    assert.equal(mq.latex(), 'x_b^{ac}');
  });

  suite('deleting', function() {
    test('backspacing out of and then re-typing subscript', function() {
      mq.latex('x_a^b');
      assert.equal(mq.latex(), 'x_a^b');

      mq.keystroke('Down Backspace');
      assert.equal(mq.latex(), 'x_{ }^b');

      mq.keystroke('Backspace');
      assert.equal(mq.latex(), 'x^b');

      mq.typedText('_a');
      assert.equal(mq.latex(), 'x_a^b');

      mq.keystroke('Left Backspace');
      assert.equal(mq.latex(), 'xa^b');

      mq.typedText('c');
      assert.equal(mq.latex(), 'xca^b');
    });
    test('backspacing out of and then re-typing superscript', function() {
      mq.latex('x_a^b');
      assert.equal(mq.latex(), 'x_a^b');

      mq.keystroke('Up Backspace');
      assert.equal(mq.latex(), 'x_a^{ }');

      mq.keystroke('Backspace');
      assert.equal(mq.latex(), 'x_a');

      mq.typedText('^b');
      assert.equal(mq.latex(), 'x_a^b');

      mq.keystroke('Left Backspace');
      assert.equal(mq.latex(), 'x_ab');

      mq.typedText('c');
      assert.equal(mq.latex(), 'x_acb');
    });
  });
});
suite('autoOperatorNames', function() {
  var mq;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  function assertLatex(input, expected) {
    var result = mq.latex();
    assert.equal(result, expected,
      input+', got \''+result+'\', expected \''+expected+'\''
    );
  }

  test('simple LaTeX parsing, typing', function() {
    function assertAutoOperatorNamesWork(str, latex) {
      var count = 0;
      var _autoUnItalicize = Letter.prototype.autoUnItalicize;
      Letter.prototype.autoUnItalicize = function() {
        count += 1;
        return _autoUnItalicize.apply(this, arguments);
      };

      mq.latex(str);
      assertLatex('parsing \''+str+'\'', latex);
      assert.equal(count, 1);

      mq.latex(latex);
      assertLatex('parsing \''+latex+'\'', latex);
      assert.equal(count, 2);

      mq.latex('');
      for (var i = 0; i < str.length; i += 1) mq.typedText(str.charAt(i));
      assertLatex('typing \''+str+'\'', latex);
      assert.equal(count, 2 + str.length);
    }

    assertAutoOperatorNamesWork('sin', '\\sin');
    assertAutoOperatorNamesWork('inf', '\\inf');
    assertAutoOperatorNamesWork('arcosh', '\\operatorname{arcosh}');
    assertAutoOperatorNamesWork('acosh', 'a\\cosh');
    assertAutoOperatorNamesWork('cosine', '\\cos ine');
    assertAutoOperatorNamesWork('arcosecant', 'ar\\operatorname{cosec}ant');
    assertAutoOperatorNamesWork('cscscscscscsc', '\\csc s\\csc s\\csc sc');
    assertAutoOperatorNamesWork('scscscscscsc', 's\\csc s\\csc s\\csc');
  });

  test('deleting', function() {
    var count = 0;
    var _autoUnItalicize = Letter.prototype.autoUnItalicize;
    Letter.prototype.autoUnItalicize = function() {
      count += 1;
      return _autoUnItalicize.apply(this, arguments);
    };

    var str = 'cscscscscscsc';
    for (var i = 0; i < str.length; i += 1) mq.typedText(str.charAt(i));
    assertLatex('typing \''+str+'\'', '\\csc s\\csc s\\csc sc');
    assert.equal(count, str.length);

    mq.moveToLeftEnd().keystroke('Del');
    assertLatex('deleted first char', 's\\csc s\\csc s\\csc');
    assert.equal(count, str.length + 1);

    mq.typedText('c');
    assertLatex('typed back first char', '\\csc s\\csc s\\csc sc');
    assert.equal(count, str.length + 2);

    mq.typedText('+');
    assertLatex('typed plus to interrupt sequence of letters', 'c+s\\csc s\\csc s\\csc');
    assert.equal(count, str.length + 4);

    mq.keystroke('Backspace');
    assertLatex('deleted plus', '\\csc s\\csc s\\csc sc');
    assert.equal(count, str.length + 5);
  });

  suite('override autoOperatorNames', function() {
    test('basic', function() {
      MQ.config({ autoOperatorNames: 'sin lol' });
      mq.typedText('arcsintrololol');
      assert.equal(mq.latex(), 'arc\\sin tro\\operatorname{lol}ol');
    });

    test('command contains non-letters', function() {
      assert.throws(function() { MQ.config({ autoOperatorNames: 'e1' }); });
    });

    test('command length less than 2', function() {
      assert.throws(function() { MQ.config({ autoOperatorNames: 'e' }); });
    });

    suite('command list not perfectly space-delimited', function() {
      test('double space', function() {
        assert.throws(function() { MQ.config({ autoOperatorNames: 'pi  theta' }); });
      });

      test('leading space', function() {
        assert.throws(function() { MQ.config({ autoOperatorNames: ' pi' }); });
      });

      test('trailing space', function() {
        assert.throws(function() { MQ.config({ autoOperatorNames: 'pi ' }); });
      });
    });
  });
});
suite('autoSubscript', function() {
  var mq;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0], {autoSubscriptNumerals: true});
    rootBlock = mq.__controller.root;
    controller = mq.__controller;
    cursor = controller.cursor;
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  test('auto subscripting variables', function() {
    mq.latex('x');
    mq.typedText('2');
    assert.equal(mq.latex(), 'x_2');
    mq.typedText('3');
    assert.equal(mq.latex(), 'x_{23}');
  });

  test('do not autosubscript functions', function() {
    mq.latex('sin');
    mq.typedText('2');
    assert.equal(mq.latex(), '\\sin2');
    mq.typedText('3');
    assert.equal(mq.latex(), '\\sin23');
  });

  test('autosubscript exponentiated variables', function() {
    mq.latex('x^2');
    mq.typedText('2');
    assert.equal(mq.latex(), 'x_2^2');
    mq.typedText('3');
    assert.equal(mq.latex(), 'x_{23}^2');
  });

  test('do not autosubscript exponentiated functions', function() {
    mq.latex('sin^{2}');
    mq.typedText('2');
    assert.equal(mq.latex(), '\\sin^22');
    mq.typedText('3');
    assert.equal(mq.latex(), '\\sin^223');
  });

  test('do not autosubscript subscripted functions', function() {
    mq.latex('sin_{10}');
    mq.typedText('2');
    assert.equal(mq.latex(), '\\sin_{10}2');
  });


  test('backspace through compound subscript', function() {
    mq.latex('x_{2_2}');

    //first backspace moves to cursor in subscript and peels it off
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');

    //second backspace clears out remaining subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{ }');

    //unpeel subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');
  });

  test('backspace through simple subscript', function() {
    mq.latex('x_{2+3}');

    assert.equal(cursor.parent, rootBlock, 'start in the root block');

    //backspace peels off subscripts but stays at the root block level
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2+}');
    assert.equal(cursor.parent, rootBlock, 'backspace keeps us in the root block');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');
    assert.equal(cursor.parent, rootBlock, 'backspace keeps us in the root block');

    //second backspace clears out remaining subscript and unpeels
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');
  });

  test('backspace through subscript & superscript with autosubscripting on', function() {
    mq.latex('x_2^{32}');

    //first backspace peels off the subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x^{32}');

    //second backspace goes into the exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x^{32}');

    //clear out exponent
    mq.keystroke('Backspace');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x^{ }');

    //unpeel exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');
  });
});
suite('backspace', function() {
  var mq, rootBlock, controller, cursor;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    rootBlock = mq.__controller.root;
    controller = mq.__controller;
    cursor = controller.cursor;
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  function prayWellFormedPoint(pt) { prayWellFormed(pt.parent, pt[L], pt[R]); }
  function assertLatex(latex) {
    prayWellFormedPoint(mq.__controller.cursor);
    assert.equal(mq.latex(), latex);
  }

  test('backspace through exponent', function() {
    controller.renderLatexMath('x^{nm}');
    var exp = rootBlock.ends[R],
      expBlock = exp.ends[L];
    assert.equal(exp.latex(), '^{nm}', 'right end el is exponent');
    assert.equal(cursor.parent, rootBlock, 'cursor is in root block');
    assert.equal(cursor[L], exp, 'cursor is at the end of root block');

    mq.keystroke('Backspace');
    assert.equal(cursor.parent, expBlock, 'cursor up goes into exponent on backspace');
    assertLatex('x^{nm}');

    mq.keystroke('Backspace');
    assert.equal(cursor.parent, expBlock, 'cursor still in exponent');
    assertLatex('x^n');

    mq.keystroke('Backspace');
    assert.equal(cursor.parent, expBlock, 'still in exponent, but it is empty');
    assertLatex('x^{ }');

    mq.keystroke('Backspace');
    assert.equal(cursor.parent, rootBlock, 'backspace tears down exponent');
    assertLatex('x');
  });

  test('backspace through complex fraction', function() {
    controller.renderLatexMath('1+\\frac{1}{\\frac{1}{2}+\\frac{2}{3}}');

    //first backspace moves to denominator
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{2}+\\frac{2}{3}}');

    //first backspace moves to denominator in denominator
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{2}+\\frac{2}{3}}');

    //finally delete a character
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{2}+\\frac{2}{ }}');

    //destroy fraction
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{2}+2}');

    mq.keystroke('Backspace');
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{2}}');

    mq.keystroke('Backspace');
    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{\\frac{1}{ }}');

    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{1}');

    mq.keystroke('Backspace');
    assertLatex('1+\\frac{1}{ }');

    mq.keystroke('Backspace');
    assertLatex('1+1');
  });



  test('backspace through compound subscript', function() {
    mq.latex('x_{2_2}');

    //first backspace goes into the subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2_2}');

    //second one goes into the subscripts' subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2_2}');

    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2_{ }}');

    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');

    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{ }');

    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');
  });

  test('backspace through simple subscript', function() {
    mq.latex('x_{2+3}');

    assert.equal(cursor.parent, rootBlock, 'start in the root block');

    //backspace goes down
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2+3}');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{2+}');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{ }');
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');
  });

  test('backspace through subscript & superscript', function() {
    mq.latex('x_2^{32}');

    //first backspace takes us into the exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2^{32}');

    //second backspace is within the exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2^3');

    //clear out exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2^{ }');

    //unpeel exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');

    //into subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_2');

    //clear out subscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x_{ }');

    //unpeel exponent
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'x');

    //clear out math field
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'');
  });

  test('backspace through nthroot', function() {
    mq.latex('\\sqrt[3]{x}');

    //first backspace takes us inside the nthroot
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'\\sqrt[3]{x}');

    //second backspace removes the x
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'\\sqrt[3]{}');

    //third one destroys the cube root, but leaves behind the 3
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'3');

    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'');
  });

  test('backspace through large operator', function() {
    mq.latex('\\sum_{n=1}^3x');

    //first backspace takes out the argument
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'\\sum_{n=1}^3');

    //up into the superscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'\\sum_{n=1}^3');

    //up into the superscript
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'\\sum_{n=1}^{ }');

    //destroy the sum, preserve the subscript (a little surprising)
    mq.keystroke('Backspace');
    assert.equal(mq.latex(),'n=1');
  });

  test('backspace into text block', function() {
    mq.latex('\\text{x}');

    mq.keystroke('Backspace');

    var textBlock = rootBlock.ends[R];
    assert.equal(cursor.parent, textBlock, 'cursor is in text block');
    assert.equal(cursor[R], 0, 'cursor is at the end of text block');
    assert.equal(cursor[L].text, 'x', 'cursor is rightward of the x');
  });

  suite('empties', function() {
    test('backspace empty exponent', function() {
      mq.latex('x^{}');
      mq.keystroke('Backspace');
      assert.equal(mq.latex(), 'x');
    });

    test('backspace empty sqrt', function() {
      mq.latex('1+\\sqrt{}');
      mq.keystroke('Backspace');
      assert.equal(mq.latex(), '1+');
    });

    test('backspace empty fraction', function() {
      mq.latex('1+\\frac{}{}');
      mq.keystroke('Backspace');
      assert.equal(mq.latex(), '1+');
    });
  });
});
suite('CSS', function() {
  test('math field doesn\'t fuck up ancestor\'s .scrollWidth', function() {
    var mock = $('#mock').css({
      fontSize: '16px',
      height: '25px', // must be greater than font-size * 115% + 2 * 2px (padding) + 2 * 1px (border)
      width: '25px'
    })[0];
    assert.equal(mock.scrollHeight, 25);
    assert.equal(mock.scrollWidth, 25);

    var mq = MQ.MathField($('<span style="box-sizing:border-box;height:100%;width:100%"></span>').appendTo(mock)[0]);
    assert.equal(mock.scrollHeight, 25);
    assert.equal(mock.scrollWidth, 25);

    $(mq.el()).remove();
    $(mock).css({
      fontSize: '',
      height: '',
      width: ''
    });
  });

  test('empty root block does not collapse', function() {
    var testEl = $('<span></span>').appendTo('#mock');
    var mq = MQ.MathField(testEl[0]);
    var rootEl = testEl.find('.mq-root-block');

    assert.ok(rootEl.hasClass('mq-empty'), 'Empty root block should have the mq-empty class name.');
    assert.ok(rootEl.height() > 0, 'Empty root block height should be above 0.');

    testEl.remove();
  });

  test('empty block does not collapse', function() {
    var testEl = $('<span>\\frac{}{}</span>').appendTo('#mock');
    var mq = MQ.MathField(testEl[0]);
    var numeratorEl = testEl.find('.mq-numerator');

    assert.ok(numeratorEl.hasClass('mq-empty'), 'Empty numerator should have the mq-empty class name.');
    assert.ok(numeratorEl.height() > 0, 'Empty numerator height should be above 0.');

    testEl.remove();
  });

  test('test florin spacing', function () {
    var mq,
        mock = $('#mock');

    mq = MathQuill.MathField($('<span></span>').appendTo(mock)[0]);
    mq.typedText("f'");

    var mqF = $(mq.el()).find('.mq-f');
    var testVal = parseFloat(mqF.css('margin-right')) - parseFloat(mqF.css('margin-left'));
    assert.ok(testVal > 0, 'this should be truthy') ;
  });
});
suite('focusBlur', function() {
  function assertHasFocus(mq, name, invert) {
    assert.ok(!!invert ^ $(mq.el()).find('textarea').is(':focus'), name + (invert ? ' does not have focus' : ' has focus'));
  }

  suite('handlers can shift focus away', function() {
    var mq, mq2, wasUpOutOfCalled;
    setup(function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0], {
        handlers: {
          upOutOf: function() {
            wasUpOutOfCalled = true;
            mq2.focus();
          }
        }
      });
      mq2 = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
      wasUpOutOfCalled = false;
    });
    teardown(function() {
      $(mq.el()).add(mq2.el()).remove();
    });

    function triggerUpOutOf(mq) {
      $(mq.el()).find('textarea').trigger(jQuery.Event('keydown', { which: 38 }));
      assert.ok(wasUpOutOfCalled);
    }

    test('normally', function() {
      mq.focus();
      assertHasFocus(mq, 'mq');

      triggerUpOutOf(mq);
      assertHasFocus(mq2, 'mq2');
    });

    test('even if there\'s a selection', function(done) {
      mq.focus();
      assertHasFocus(mq, 'mq');

      mq.typedText('asdf');
      assert.equal(mq.latex(), 'asdf');

      mq.keystroke('Shift-Left');
      setTimeout(function() {
        assert.equal($(mq.el()).find('textarea').val(), 'f');

        triggerUpOutOf(mq);
        assertHasFocus(mq2, 'mq2');
        done();
      });
    });
  });

  test('select behaves normally after blurring and re-focusing', function(done) {
    var mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);

    mq.focus();
    assertHasFocus(mq, 'mq');

    mq.typedText('asdf');
    assert.equal(mq.latex(), 'asdf');

    mq.keystroke('Shift-Left');
    setTimeout(function() {
      assert.equal($(mq.el()).find('textarea').val(), 'f');

      mq.blur();
      assertHasFocus(mq, 'mq', 'not');
      setTimeout(function() {
        assert.equal($(mq.el()).find('textarea').val(), '');

        mq.focus();
        assertHasFocus(mq, 'mq');

        mq.keystroke('Shift-Left');
        setTimeout(function() {
          assert.equal($(mq.el()).find('textarea').val(), 'd');

          $(mq.el()).remove();
          done();
        });
      }, 10);
    });
  });
});
suite('HTML', function() {
  function renderHtml(numBlocks, htmlTemplate) {
    var cmd = {
      id: 1,
      blocks: Array(numBlocks),
      htmlTemplate: htmlTemplate
    };
    for (var i = 0; i < numBlocks; i += 1) {
      cmd.blocks[i] = {
        i: i,
        id: 2 + i,
        join: function() { return 'Block:' + this.i; }
      };
    }
    return MathCommand.prototype.html.call(cmd);
  }

  test('simple HTML templates', function() {
    var htmlTemplate = '<span>A Symbol</span>';
    var html = '<span mathquill-command-id=1>A Symbol</span>';

    assert.equal(html, renderHtml(0, htmlTemplate), 'a symbol');

    htmlTemplate = '<span>&0</span>';
    html = '<span mathquill-command-id=1 mathquill-block-id=2>Block:0</span>';

    assert.equal(html, renderHtml(1, htmlTemplate), 'same span is cmd and block');

    htmlTemplate =
        '<span>'
      +   '<span>&0</span>'
      +   '<span>&1</span>'
      + '</span>'
    ;
    html =
        '<span mathquill-command-id=1>'
      +   '<span mathquill-block-id=2>Block:0</span>'
      +   '<span mathquill-block-id=3>Block:1</span>'
      + '</span>'
    ;

    assert.equal(html, renderHtml(2, htmlTemplate), 'container span with two block spans');
  });

  test('context-free HTML templates', function() {
    var htmlTemplate = '<br/>';
    var html = '<br mathquill-command-id=1/>';

    assert.equal(html, renderHtml(0, htmlTemplate), 'self-closing tag');

    htmlTemplate =
        '<span>'
      +   '<span>&0</span>'
      + '</span>'
      + '<span>'
      +   '<span>&1</span>'
      + '</span>'
    ;
    html =
        '<span mathquill-command-id=1>'
      +   '<span mathquill-block-id=2>Block:0</span>'
      + '</span>'
      + '<span mathquill-command-id=1>'
      +   '<span mathquill-block-id=3>Block:1</span>'
      + '</span>'
    ;

    assert.equal(html, renderHtml(2, htmlTemplate), 'two cmd spans');

    htmlTemplate =
        '<span></span>'
      + '<span/>'
      + '<span>'
      +   '<span>'
      +     '<span/>'
      +   '</span>'
      +   '<span>&1</span>'
      +   '<span/>'
      +   '<span></span>'
      + '</span>'
      + '<span>&0</span>'
    ;
    html =
        '<span mathquill-command-id=1></span>'
      + '<span mathquill-command-id=1/>'
      + '<span mathquill-command-id=1>'
      +   '<span>'
      +     '<span/>'
      +   '</span>'
      +   '<span mathquill-block-id=3>Block:1</span>'
      +   '<span/>'
      +   '<span></span>'
      + '</span>'
      + '<span mathquill-command-id=1 mathquill-block-id=2>Block:0</span>'
    ;

    assert.equal(html, renderHtml(2, htmlTemplate), 'multiple nested cmd and block spans');
  });
});
suite('latex', function() {
  function assertParsesLatex(str, latex) {
    if (arguments.length < 2) latex = str;

    var result = latexMathParser.parse(str).postOrder('finalizeTree', Options.p).join('latex');
    assert.equal(result, latex,
      'parsing \''+str+'\', got \''+result+'\', expected \''+latex+'\''
    );
  }

  test('empty LaTeX', function () {
    assertParsesLatex('');
    assertParsesLatex(' ', '');
    assertParsesLatex('{}', '');
    assertParsesLatex('   {}{} {{{}}  }', '');
  });

  test('variables', function() {
    assertParsesLatex('xyz');
  });

  test('variables that can be mathbb', function() {
    assertParsesLatex('PNZQRCH');
  });

  test('simple exponent', function() {
    assertParsesLatex('x^n');
  });

  test('block exponent', function() {
    assertParsesLatex('x^{n}', 'x^n');
    assertParsesLatex('x^{nm}');
    assertParsesLatex('x^{}', 'x^{ }');
  });

  test('nested exponents', function() {
    assertParsesLatex('x^{n^m}');
  });

  test('exponents with spaces', function() {
    assertParsesLatex('x^ 2', 'x^2');

    assertParsesLatex('x ^2', 'x^2');
  });

  test('inner groups', function() {
    assertParsesLatex('a{bc}d', 'abcd');
    assertParsesLatex('{bc}d', 'bcd');
    assertParsesLatex('a{bc}', 'abc');
    assertParsesLatex('{bc}', 'bc');

    assertParsesLatex('x^{a{bc}d}', 'x^{abcd}');
    assertParsesLatex('x^{a{bc}}', 'x^{abc}');
    assertParsesLatex('x^{{bc}}', 'x^{bc}');
    assertParsesLatex('x^{{bc}d}', 'x^{bcd}');

    assertParsesLatex('{asdf{asdf{asdf}asdf}asdf}', 'asdfasdfasdfasdfasdf');
  });

  test('commands without braces', function() {
    assertParsesLatex('\\frac12', '\\frac{1}{2}');
    assertParsesLatex('\\frac1a', '\\frac{1}{a}');
    assertParsesLatex('\\frac ab', '\\frac{a}{b}');

    assertParsesLatex('\\frac a b', '\\frac{a}{b}');
    assertParsesLatex(' \\frac a b ', '\\frac{a}{b}');
    assertParsesLatex('\\frac{1} 2', '\\frac{1}{2}');
    assertParsesLatex('\\frac{ 1 } 2', '\\frac{1}{2}');

    assert.throws(function() { latexMathParser.parse('\\frac'); });
  });

  test('whitespace', function() {
    assertParsesLatex('  a + b ', 'a+b');
    assertParsesLatex('       ', '');
    assertParsesLatex('', '');
  });

  test('parens', function() {
    var tree = latexMathParser.parse('\\left(123\\right)');

    assert.ok(tree.ends[L] instanceof Bracket);
    var contents = tree.ends[L].ends[L].join('latex');
    assert.equal(contents, '123');
    assert.equal(tree.join('latex'), '\\left(123\\right)');
  });

  test('parens with whitespace', function() {
    assertParsesLatex('\\left ( 123 \\right ) ', '\\left(123\\right)');
  });

  test('escaped whitespace', function() {
    assertParsesLatex('\\ ', '\\ ');
    assertParsesLatex('\\      ', '\\ ');
    assertParsesLatex('  \\   \\\t\t\t\\   \\\n\n\n', '\\ \\ \\ \\ ');
    assertParsesLatex('\\space\\   \\   space  ', '\\ \\ \\ space');
  });

  test('\\text', function() {
    assertParsesLatex('\\text { lol! } ', '\\text{ lol! }');
    assertParsesLatex('\\text{apples} \\ne \\text{oranges}',
                      '\\text{apples}\\ne \\text{oranges}');
  });

  test('not real LaTex commands, but valid symbols', function() {
    assertParsesLatex('\\parallelogram ');
    assertParsesLatex('\\circledot ', '\\odot ');
    assertParsesLatex('\\degree ');
    assertParsesLatex('\\square ');
  });

  suite('public API', function() {
    var mq;
    setup(function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    });
    teardown(function() {
      $(mq.el()).remove();
    });

    suite('.latex(...)', function() {
      function assertParsesLatex(str, latex) {
        if (arguments.length < 2) latex = str;
        mq.latex(str);
        assert.equal(mq.latex(), latex);
      }

      test('basic rendering', function() {
        assertParsesLatex('x = \\frac{ -b \\pm \\sqrt{ b^2 - 4ac } }{ 2a }',
                          'x=\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}');
      });

      test('re-rendering', function() {
        assertParsesLatex('a x^2 + b x + c = 0', 'ax^2+bx+c=0');
        assertParsesLatex('x = \\frac{ -b \\pm \\sqrt{ b^2 - 4ac } }{ 2a }',
                          'x=\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}');
      });

      test('empty LaTeX', function () {
        assertParsesLatex('');
        assertParsesLatex(' ', '');
        assertParsesLatex('{}', '');
        assertParsesLatex('   {}{} {{{}}  }', '');
      });

      test('coerces to a string', function () {
        assertParsesLatex(undefined, 'undefined');
        assertParsesLatex(null, 'null');
        assertParsesLatex(0, '0');
        assertParsesLatex(Infinity, 'Infinity');
        assertParsesLatex(NaN, 'NaN');
        assertParsesLatex(true, 'true');
        assertParsesLatex(false, 'false');
        assertParsesLatex({}, '[objectObject]'); // lol, the space gets ignored
        assertParsesLatex({toString: function() { return 'thing'; }}, 'thing');
      });
    });

    suite('.write(...)', function() {
      test('empty LaTeX', function () {
        function assertParsesLatex(str, latex) {
          if (arguments.length < 2) latex = str;
          mq.write(str);
          assert.equal(mq.latex(), latex);
        }
        assertParsesLatex('');
        assertParsesLatex(' ', '');
        assertParsesLatex('{}', '');
        assertParsesLatex('   {}{} {{{}}  }', '');
      });

      test('overflow triggers automatic horizontal scroll', function(done) {
        var mqEl = mq.el();
        var rootEl = mq.__controller.root.jQ[0];
        var cursor = mq.__controller.cursor;

        $(mqEl).width(10);
        var previousScrollLeft = rootEl.scrollLeft;

        mq.write("abc");
        setTimeout(afterScroll, 150);

        function afterScroll() {
          cursor.show();

          try {
            assert.ok(rootEl.scrollLeft > previousScrollLeft, "scrolls on write");
            assert.ok(mqEl.getBoundingClientRect().right > cursor.jQ[0].getBoundingClientRect().right,
              "cursor right end is inside the field");
          }
          catch(error) {
            done(error);
            return;
          }

          done();
        }
      });

      suite('\\sum', function() {
        test('basic', function() {
          mq.write('\\sum_{n=0}^5');
          assert.equal(mq.latex(), '\\sum_{n=0}^5');
          mq.write('x^n');
          assert.equal(mq.latex(), '\\sum_{n=0}^5x^n');
        });

        test('only lower bound', function() {
          mq.write('\\sum_{n=0}');
          assert.equal(mq.latex(), '\\sum_{n=0}^{ }');
          mq.write('x^n');
          assert.equal(mq.latex(), '\\sum_{n=0}^{ }x^n');
        });

        test('only upper bound', function() {
          mq.write('\\sum^5');
          assert.equal(mq.latex(), '\\sum_{ }^5');
          mq.write('x^n');
          assert.equal(mq.latex(), '\\sum_{ }^5x^n');
        });
      });
    });
  });

  suite('\\MathQuillMathField', function() {
    var outer, inner1, inner2;
    setup(function() {
      outer = MQ.StaticMath(
        $('<span>\\frac{\\MathQuillMathField{x_0 + x_1 + x_2}}{\\MathQuillMathField{3}}</span>')
        .appendTo('#mock')[0]
      );
      inner1 = outer.innerFields[0];
      inner2 = outer.innerFields[1];
    });
    teardown(function() {
      $(outer.el()).remove();
    });

    test('initial latex', function() {
      assert.equal(inner1.latex(), 'x_0+x_1+x_2');
      assert.equal(inner2.latex(), '3');
      assert.equal(outer.latex(), '\\frac{x_0+x_1+x_2}{3}');
    });

    test('setting latex', function() {
      inner1.latex('\\sum_{i=0}^N x_i');
      inner2.latex('N');
      assert.equal(inner1.latex(), '\\sum_{i=0}^Nx_i');
      assert.equal(inner2.latex(), 'N');
      assert.equal(outer.latex(), '\\frac{\\sum_{i=0}^Nx_i}{N}');
    });

    test('writing latex', function() {
      inner1.write('+ x_3');
      inner2.write('+ 1');
      assert.equal(inner1.latex(), 'x_0+x_1+x_2+x_3');
      assert.equal(inner2.latex(), '3+1');
      assert.equal(outer.latex(), '\\frac{x_0+x_1+x_2+x_3}{3+1}');
    });

    test('optional inner field name', function() {
      outer.latex('\\MathQuillMathField[mantissa]{}\\cdot\\MathQuillMathField[base]{}^{\\MathQuillMathField[exp]{}}');
      assert.equal(outer.innerFields.length, 3);

      var mantissa = outer.innerFields.mantissa;
      var base = outer.innerFields.base;
      var exp = outer.innerFields.exp;

      assert.equal(mantissa, outer.innerFields[0]);
      assert.equal(base, outer.innerFields[1]);
      assert.equal(exp, outer.innerFields[2]);

      mantissa.latex('1.2345');
      base.latex('10');
      exp.latex('8');
      assert.equal(outer.latex(), '1.2345\\cdot10^8');
    });

    test('separate API object', function() {
      var outer2 = MQ(outer.el());
      assert.equal(outer2.innerFields.length, 2);
      assert.equal(outer2.innerFields[0].id, inner1.id);
      assert.equal(outer2.innerFields[1].id, inner2.id);
    });
  });

  suite('error handling', function() {
    var mq;
    setup(function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    });
    teardown(function() {
      $(mq.el()).remove();
    });

    function testCantParse(title /*, latex...*/) {
      var latex = [].slice.call(arguments, 1);
      test(title, function() {
        for (var i = 0; i < latex.length; i += 1) {
          mq.latex(latex[i]);
          assert.equal(mq.latex(), '', "shouldn\'t parse '"+latex[i]+"'");
        }
      });
    }

    testCantParse('missing blocks', '\\frac', '\\sqrt', '^', '_');
    testCantParse('unmatched close brace', '}', ' 1 + 2 } ', '1 - {2 + 3} }', '\\sqrt{ x }} + \\sqrt{y}');
    testCantParse('unmatched open brace', '{', '1 * { 2 + 3', '\\frac{ \\sqrt x }{{ \\sqrt y}');
    testCantParse('unmatched \\left/\\right', '\\left ( 1 + 2 )', ' [ 1, 2 \\right ]');
  });
});
suite('parser', function() {
  var string = Parser.string;
  var regex = Parser.regex;
  var letter = Parser.letter;
  var digit = Parser.digit;
  var any = Parser.any;
  var optWhitespace = Parser.optWhitespace;
  var eof = Parser.eof;
  var succeed = Parser.succeed;
  var all = Parser.all;

  test('Parser.string', function() {
    var parser = string('x');
    assert.equal(parser.parse('x'), 'x');
    assert.throws(function() { parser.parse('y') })
  });

  test('Parser.regex', function() {
    var parser = regex(/^[0-9]/);

    assert.equal(parser.parse('1'), '1');
    assert.equal(parser.parse('4'), '4');
    assert.throws(function() { parser.parse('x'); });
    assert.throws(function() { regex(/./) }, 'must be anchored');
  });

  suite('then', function() {
    test('with a parser, uses the last return value', function() {
      var parser = string('x').then(string('y'));
      assert.equal(parser.parse('xy'), 'y');
      assert.throws(function() { parser.parse('y'); });
      assert.throws(function() { parser.parse('xz'); });
    });

    test('asserts that a parser is returned', function() {
      var parser1 = letter.then(function() { return 'not a parser' });
      assert.throws(function() { parser1.parse('x'); });

      var parser2 = letter.then('x');
      assert.throws(function() { letter.parse('xx'); });
    });

    test('with a function that returns a parser, continues with that parser', function() {
      var piped;
      var parser = string('x').then(function(x) {
        piped = x;
        return string('y');
      });

      assert.equal(parser.parse('xy'), 'y');
      assert.equal(piped, 'x');
      assert.throws(function() { parser.parse('x'); });
    });
  });

  suite('map', function() {
    test('with a function, pipes the value in and uses that return value', function() {
      var piped;

      var parser = string('x').map(function(x) {
        piped = x;
        return 'y';
      });

      assert.equal(parser.parse('x'), 'y')
      assert.equal(piped, 'x');
    });
  });

  suite('result', function() {
    test('returns a constant result', function() {
      var myResult = 1;
      var oneParser = string('x').result(1);

      assert.equal(oneParser.parse('x'), 1);

      var myFn = function() {};
      var fnParser = string('x').result(myFn);

      assert.equal(fnParser.parse('x'), myFn);
    });
  });

  suite('skip', function() {
    test('uses the previous return value', function() {
      var parser = string('x').skip(string('y'));

      assert.equal(parser.parse('xy'), 'x');
      assert.throws(function() { parser.parse('x'); });
    });
  });

  suite('or', function() {
    test('two parsers', function() {
      var parser = string('x').or(string('y'));

      assert.equal(parser.parse('x'), 'x');
      assert.equal(parser.parse('y'), 'y');
      assert.throws(function() { parser.parse('z') });
    });

    test('with then', function() {
      var parser = string('\\')
        .then(function() {
          return string('y')
        }).or(string('z'));

      assert.equal(parser.parse('\\y'), 'y');
      assert.equal(parser.parse('z'), 'z');
      assert.throws(function() { parser.parse('\\z') });
    });
  });

  function assertEqualArray(arr1, arr2) {
    assert.equal(arr1.join(), arr2.join());
  }

  suite('many', function() {
    test('simple case', function() {
      var letters = letter.many();

      assertEqualArray(letters.parse('x'), ['x']);
      assertEqualArray(letters.parse('xyz'), ['x','y','z']);
      assertEqualArray(letters.parse(''), []);
      assert.throws(function() { letters.parse('1'); });
      assert.throws(function() { letters.parse('xyz1'); });
    });

    test('followed by then', function() {
      var parser = string('x').many().then(string('y'));

      assert.equal(parser.parse('y'), 'y');
      assert.equal(parser.parse('xy'), 'y');
      assert.equal(parser.parse('xxxxxy'), 'y');
    });
  });

  suite('times', function() {
    test('zero case', function() {
      var zeroLetters = letter.times(0);

      assertEqualArray(zeroLetters.parse(''), []);
      assert.throws(function() { zeroLetters.parse('x'); });
    });

    test('nonzero case', function() {
      var threeLetters = letter.times(3);

      assertEqualArray(threeLetters.parse('xyz'), ['x', 'y', 'z']);
      assert.throws(function() { threeLetters.parse('xy'); });
      assert.throws(function() { threeLetters.parse('xyzw'); });

      var thenDigit = threeLetters.then(digit);
      assert.equal(thenDigit.parse('xyz1'), '1');
      assert.throws(function() { thenDigit.parse('xy1'); });
      assert.throws(function() { thenDigit.parse('xyz'); });
      assert.throws(function() { thenDigit.parse('xyzw'); });
    });

    test('with a min and max', function() {
      var someLetters = letter.times(2, 4);

      assertEqualArray(someLetters.parse('xy'), ['x', 'y']);
      assertEqualArray(someLetters.parse('xyz'), ['x', 'y', 'z']);
      assertEqualArray(someLetters.parse('xyzw'), ['x', 'y', 'z', 'w']);
      assert.throws(function() { someLetters.parse('xyzwv'); });
      assert.throws(function() { someLetters.parse('x'); });

      var thenDigit = someLetters.then(digit);
      assert.equal(thenDigit.parse('xy1'), '1');
      assert.equal(thenDigit.parse('xyz1'), '1');
      assert.equal(thenDigit.parse('xyzw1'), '1');
      assert.throws(function() { thenDigit.parse('xy'); });
      assert.throws(function() { thenDigit.parse('xyzw'); });
      assert.throws(function() { thenDigit.parse('xyzwv1'); });
      assert.throws(function() { thenDigit.parse('x1'); });
    });

    test('atLeast', function() {
      var atLeastTwo = letter.atLeast(2);

      assertEqualArray(atLeastTwo.parse('xy'), ['x', 'y']);
      assertEqualArray(atLeastTwo.parse('xyzw'), ['x', 'y', 'z', 'w']);
      assert.throws(function() { atLeastTwo.parse('x'); });
    });
  });

  suite('fail', function() {
    var fail = Parser.fail;
    var succeed = Parser.succeed;

    test('use Parser.fail to fail dynamically', function() {
      var parser = any.then(function(ch) {
        return fail('character '+ch+' not allowed');
      }).or(string('x'));

      assert.throws(function() { parser.parse('y'); });
      assert.equal(parser.parse('x'), 'x');
    });

    test('use Parser.succeed or Parser.fail to branch conditionally', function() {
      var allowedOperator;

      var parser =
        string('x')
        .then(string('+').or(string('*')))
        .then(function(operator) {
          if (operator === allowedOperator) return succeed(operator);
          else return fail('expected '+allowedOperator);
        })
        .skip(string('y'))
      ;

      allowedOperator = '+';
      assert.equal(parser.parse('x+y'), '+');
      assert.throws(function() { parser.parse('x*y'); });

      allowedOperator = '*';
      assert.equal(parser.parse('x*y'), '*');
      assert.throws(function() { parser.parse('x+y'); });
    });
  });

  test('eof', function() {
    var parser = optWhitespace.skip(eof).or(all.result('default'));

    assert.equal(parser.parse('  '), '  ')
    assert.equal(parser.parse('x'), 'default');
  });
});
suite('Public API', function() {
  suite('global functions', function() {
    test('null', function() {
      assert.equal(MQ(), null);
      assert.equal(MQ(0), null);
      assert.equal(MQ('<span/>'), null);
      assert.equal(MQ($('<span/>')[0]), null);
      assert.equal(MQ.MathField(), null);
      assert.equal(MQ.MathField(0), null);
      assert.equal(MQ.MathField('<span/>'), null);
    });

    test('MQ.MathField()', function() {
      var el = $('<span>x^2</span>');
      var mathField = MQ.MathField(el[0]);
      assert.ok(mathField instanceof MQ.MathField);
      assert.ok(mathField instanceof MQ.EditableField);
      assert.ok(mathField instanceof MQ);
      assert.ok(mathField instanceof MathQuill);
    });

    test('interface versioning isolates prototype chain', function() {
      var mathFieldSpan = $('<span/>')[0];
      var mathField = MQ.MathField(mathFieldSpan);

      var MQ1 = MathQuill.getInterface(1);
      assert.ok(!(mathField instanceof MQ1.MathField));
      assert.ok(!(mathField instanceof MQ1.EditableField));
      assert.ok(!(mathField instanceof MQ1));
    });

    test('identity of API object returned by MQ()', function() {
      var mathFieldSpan = $('<span/>')[0];
      var mathField = MQ.MathField(mathFieldSpan);

      assert.ok(MQ(mathFieldSpan) !== mathField);

      assert.equal(MQ(mathFieldSpan).id, mathField.id);
      assert.equal(MQ(mathFieldSpan).id, MQ(mathFieldSpan).id);

      assert.equal(MQ(mathFieldSpan).data, mathField.data);
      assert.equal(MQ(mathFieldSpan).data, MQ(mathFieldSpan).data);
    });

    test('blurred when created', function() {
      var el = $('<span/>');
      MQ.MathField(el[0]);
      var rootBlock = el.find('.mq-root-block');
      assert.ok(rootBlock.hasClass('mq-empty'));
      assert.ok(!rootBlock.hasClass('mq-hasCursor'));
    });
  });

  suite('mathquill-basic', function() {
    var mq;
    setup(function() {
      mq = MQBasic.MathField($('<span></span>').appendTo('#mock')[0]);
    });
    teardown(function() {
      $(mq.el()).remove();
    });

    test('typing \\', function() {
      mq.typedText('\\');
      assert.equal(mq.latex(), '\\backslash');
    });

    test('typing $', function() {
      mq.typedText('$');
      assert.equal(mq.latex(), '\\$');
    });

    test('parsing of advanced symbols', function() {
      mq.latex('\\oplus');
      assert.equal(mq.latex(), ''); // TODO: better LaTeX parse error behavior
    });
  });

  suite('basic API methods', function() {
    var mq;
    setup(function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    });
    teardown(function() {
      $(mq.el()).remove();
    });

    test('.revert()', function() {
      var mq = MQ.MathField($('<span>some <code>HTML</code></span>')[0]);
      assert.equal(mq.revert().html(), 'some <code>HTML</code>');
    });

    test('select, clearSelection', function() {
      mq.latex('n+\\frac{n}{2}');
      assert.ok(!mq.__controller.cursor.selection);
      mq.select();
      assert.equal(mq.__controller.cursor.selection.join('latex'), 'n+\\frac{n}{2}');
      mq.clearSelection();
      assert.ok(!mq.__controller.cursor.selection);
    });

    test('latex while there\'s a selection', function() {
      mq.latex('a');
      assert.equal(mq.latex(), 'a');
      mq.select();
      assert.equal(mq.__controller.cursor.selection.join('latex'), 'a');
      mq.latex('b');
      assert.equal(mq.latex(), 'b');
      mq.typedText('c');
      assert.equal(mq.latex(), 'bc');
    });

    test('.html() trivial case', function() {
      mq.latex('x+y');
      assert.equal(mq.html(), '<var>x</var><span class="mq-binary-operator">+</span><var>y</var>');
    });
    
    test('.text() with incomplete commands', function() {
      assert.equal(mq.text(), '');
      mq.typedText('\\');
      assert.equal(mq.text(), '\\');
      mq.typedText('s');
      assert.equal(mq.text(), '\\s');
      mq.typedText('qrt');
      assert.equal(mq.text(), '\\sqrt');
    });

    test('.text() with complete commands', function() {
      mq.latex('\\sqrt{}');
      assert.equal(mq.text(), 'sqrt()');
      mq.latex('\\nthroot[]{}');
      assert.equal(mq.text(), 'sqrt[]()');
      mq.latex('\\frac{}{}');
      assert.equal(mq.text(), '()/()');
      mq.latex('\\frac{3}{5}');
      assert.equal(mq.text(), '(3)/(5)');
      mq.latex('\\frac{3+2}{5-1}');
      assert.equal(mq.text(), '(3+2)/(5-1)');
      mq.latex('\\div');
      assert.equal(mq.text(), '[/]');
      mq.latex('^{}');
      assert.equal(mq.text(), '^');
      mq.latex('3^{4}');
      assert.equal(mq.text(), '3^4');
      mq.latex('3x+\\ 4');
      assert.equal(mq.text(), '3*x+ 4');
      mq.latex('x^2');
      assert.equal(mq.text(), 'x^2');

      mq.latex('');
      mq.typedText('*2*3***4');
      assert.equal(mq.text(), '*2*3***4');
    });

    test('.moveToDirEnd(dir)', function() {
      mq.latex('a x^2 + b x + c = 0');
      assert.equal(mq.__controller.cursor[L].ctrlSeq, '0');
      assert.equal(mq.__controller.cursor[R], 0);
      mq.moveToLeftEnd();
      assert.equal(mq.__controller.cursor[L], 0);
      assert.equal(mq.__controller.cursor[R].ctrlSeq, 'a');
      mq.moveToRightEnd();
      assert.equal(mq.__controller.cursor[L].ctrlSeq, '0');
      assert.equal(mq.__controller.cursor[R], 0);
    });
  });

  test('edit handler interface versioning', function() {
    var count = 0;

    // interface version 2 (latest)
    var mq2 = MQ.MathField($('<span></span>').appendTo('#mock')[0], {
      handlers: {
        edit: function(_mq) {
          assert.equal(mq2.id, _mq.id);
          count += 1;
        }
      }
    });
    assert.equal(count, 0);
    mq2.latex('x^2');
    assert.equal(count, 2); // sigh, once for postOrder and once for bubble

    count = 0;
    // interface version 1
    var MQ1 = MathQuill.getInterface(1);
    var mq1 = MQ1.MathField($('<span></span>').appendTo('#mock')[0], {
      handlers: {
        edit: function(_mq) {
          if (count <= 2) assert.equal(mq1, undefined);
          else assert.equal(mq1.id, _mq.id);
          count += 1;
        }
      }
    });
    assert.equal(count, 2);
  });

  suite('*OutOf handlers', function() {
    testHandlers('MQ.MathField() constructor', function(options) {
      return MQ.MathField($('<span></span>').appendTo('#mock')[0], options);
    });
    testHandlers('MQ.MathField::config()', function(options) {
      return MQ.MathField($('<span></span>').appendTo('#mock')[0]).config(options);
    });
    testHandlers('.config() on \\MathQuillMathField{} in a MQ.StaticMath', function(options) {
      return MQ.MathField($('<span></span>').appendTo('#mock')[0]).config(options);
    });
    suite('global MQ.config()', function() {
      testHandlers('a MQ.MathField', function(options) {
        MQ.config(options);
        return MQ.MathField($('<span></span>').appendTo('#mock')[0]);
      });
      testHandlers('\\MathQuillMathField{} in a MQ.StaticMath', function(options) {
        MQ.config(options);
        return MQ.StaticMath($('<span>\\MathQuillMathField{}</span>').appendTo('#mock')[0]).innerFields[0];
      });
      teardown(function() {
        MQ.config({ handlers: undefined });
      });
    });
    function testHandlers(title, mathFieldMaker) {
      test(title, function() {
        var enterCounter = 0, upCounter = 0, moveCounter = 0, deleteCounter = 0,
          dir = null;

        var mq = mathFieldMaker({
          handlers: {
            enter: function(_mq) {
              assert.equal(arguments.length, 1);
              assert.equal(_mq.id, mq.id);
              enterCounter += 1;
            },
            upOutOf: function(_mq) {
              assert.equal(arguments.length, 1);
              assert.equal(_mq.id, mq.id);
              upCounter += 1;
            },
            moveOutOf: function(_dir, _mq) {
              assert.equal(arguments.length, 2);
              assert.equal(_mq.id, mq.id);
              dir = _dir;
              moveCounter += 1;
            },
            deleteOutOf: function(_dir, _mq) {
              assert.equal(arguments.length, 2);
              assert.equal(_mq.id, mq.id);
              dir = _dir;
              deleteCounter += 1;
            }
          }
        });

        mq.latex('n+\\frac{n}{2}'); // starts at right edge
        assert.equal(moveCounter, 0);

        mq.typedText('\n'); // nothing happens
        assert.equal(enterCounter, 1);

        mq.keystroke('Right'); // stay at right edge
        assert.equal(moveCounter, 1);
        assert.equal(dir, R);

        mq.keystroke('Right'); // stay at right edge
        assert.equal(moveCounter, 2);
        assert.equal(dir, R);

        mq.keystroke('Left'); // right edge of denominator
        assert.equal(moveCounter, 2);
        assert.equal(upCounter, 0);

        mq.keystroke('Up'); // right edge of numerator
        assert.equal(moveCounter, 2);
        assert.equal(upCounter, 0);

        mq.keystroke('Up'); // stays at right edge of numerator
        assert.equal(upCounter, 1);

        mq.keystroke('Up'); // stays at right edge of numerator
        assert.equal(upCounter, 2);

        // go to left edge
        mq.keystroke('Left').keystroke('Left').keystroke('Left').keystroke('Left');
        assert.equal(moveCounter, 2);

        mq.keystroke('Left'); // stays at left edge
        assert.equal(moveCounter, 3);
        assert.equal(dir, L);
        assert.equal(deleteCounter, 0);

        mq.keystroke('Backspace'); // stays at left edge
        assert.equal(deleteCounter, 1);
        assert.equal(dir, L);

        mq.keystroke('Backspace'); // stays at left edge
        assert.equal(deleteCounter, 2);
        assert.equal(dir, L);

        mq.keystroke('Left'); // stays at left edge
        assert.equal(moveCounter, 4);
        assert.equal(dir, L);

        $('#mock').empty();
      });
    }
  });

  suite('.cmd(...)', function() {
    var mq;
    setup(function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    });
    teardown(function() {
      $(mq.el()).remove();
    });

    test('basic', function() {
      mq.cmd('x');
      assert.equal(mq.latex(), 'x');
      mq.cmd('y');
      assert.equal(mq.latex(), 'xy');
      mq.cmd('^');
      assert.equal(mq.latex(), 'xy^{ }');
      mq.cmd('2');
      assert.equal(mq.latex(), 'xy^2');
      mq.keystroke('Right Shift-Left Shift-Left Shift-Left').cmd('\\sqrt');
      assert.equal(mq.latex(), '\\sqrt{xy^2}');
      mq.typedText('*2**');
      assert.equal(mq.latex(), '\\sqrt{xy^2\\cdot2\\cdot\\cdot}');
    });

    test('backslash commands are passed their name', function() {
      mq.cmd('\\alpha');
      assert.equal(mq.latex(), '\\alpha');
    });

    test('replaces selection', function() {
      mq.typedText('49').select().cmd('\\sqrt');
      assert.equal(mq.latex(), '\\sqrt{49}');
    });

    test('operator name', function() {
      mq.cmd('\\sin');
      assert.equal(mq.latex(), '\\sin');
    });

    test('nonexistent LaTeX command is noop', function() {
      mq.typedText('49').select().cmd('\\asdf').cmd('\\sqrt');
      assert.equal(mq.latex(), '\\sqrt{49}');
    });

    test('overflow triggers automatic horizontal scroll', function(done) {
      var mqEl = mq.el();
      var rootEl = mq.__controller.root.jQ[0];
      var cursor = mq.__controller.cursor;

      $(mqEl).width(10);
      var previousScrollLeft = rootEl.scrollLeft;

      mq.cmd("\\alpha");
      setTimeout(afterScroll, 150);

      function afterScroll() {
        cursor.show();

        try {
          assert.ok(rootEl.scrollLeft > previousScrollLeft, "scrolls on cmd");
          assert.ok(mqEl.getBoundingClientRect().right > cursor.jQ[0].getBoundingClientRect().right,
            "cursor right end is inside the field");
        }
        catch(error) {
          done(error);
          return;
        }

        done();
      }
    });
  });

  suite('spaceBehavesLikeTab', function() {
    var mq, rootBlock, cursor;
    test('space behaves like tab with default opts', function() {
      mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
      rootBlock = mq.__controller.root;
      cursor = mq.__controller.cursor;

      mq.latex('\\sqrt{x}');
      mq.keystroke('Left');

      mq.keystroke('Spacebar');
      mq.typedText(' ');
      assert.equal(cursor[L].ctrlSeq, '\\ ', 'left of the cursor is ' + cursor[L].ctrlSeq);
      assert.equal(cursor[R], 0, 'right of the cursor is ' + cursor[R]);
      mq.keystroke('Backspace');

      mq.keystroke('Shift-Spacebar');
      mq.typedText(' ');
      assert.equal(cursor[L].ctrlSeq, '\\ ', 'left of the cursor is ' + cursor[L].ctrlSeq);
      assert.equal(cursor[R], 0, 'right of the cursor is ' + cursor[R]);

      $(mq.el()).remove();
    });
    test('space behaves like tab when spaceBehavesLikeTab is true', function() {
      var opts = { 'spaceBehavesLikeTab': true };
      mq = MQ.MathField( $('<span></span>').appendTo('#mock')[0], opts)
      rootBlock = mq.__controller.root;
      cursor = mq.__controller.cursor;

      mq.latex('\\sqrt{x}');

      mq.keystroke('Left');
      mq.keystroke('Spacebar');
      assert.equal(cursor[L].parent, rootBlock, 'parent of the cursor is  ' + cursor[L].ctrlSeq);
      assert.equal(cursor[R], 0, 'right cursor is ' + cursor[R]);

      mq.keystroke('Left');
      mq.keystroke('Shift-Spacebar');
      assert.equal(cursor[L], 0, 'left cursor is ' + cursor[L]);
      assert.equal(cursor[R], rootBlock.ends[L], 'parent of rootBlock is ' + cursor[R]);

      $(mq.el()).remove();
    });
    test('space behaves like tab when globally set to true', function() {
      MQ.config({ spaceBehavesLikeTab: true });

      mq = MQ.MathField( $('<span></span>').appendTo('#mock')[0]);
      rootBlock = mq.__controller.root;
      cursor = mq.__controller.cursor;

      mq.latex('\\sqrt{x}');

      mq.keystroke('Left');
      mq.keystroke('Spacebar');
      assert.equal(cursor.parent, rootBlock, 'cursor in root block');
      assert.equal(cursor[R], 0, 'cursor at end of block');

      $(mq.el()).remove();
    });
  });

  suite('statelessClipboard option', function() {
    suite('default', function() {
      var mq, textarea;
      setup(function() {
        mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
        textarea = $(mq.el()).find('textarea');;
      });
      teardown(function() {
        $(mq.el()).remove();
      });
      function assertPaste(paste, latex) {
        if (arguments.length < 2) latex = paste;
        mq.latex('');
        textarea.trigger('paste').val(paste).trigger('input');
        assert.equal(mq.latex(), latex);
      }

      test('numbers and letters', function() {
        assertPaste('123xyz');
      });
      test('a sentence', function() {
        assertPaste('Lorem ipsum is a placeholder text commonly used to '
                    + 'demonstrate the graphical elements of a document or '
                    + 'visual presentation.',
                    'Loremipsumisaplaceholdertextcommonlyusedtodemonstrate'
                    + 'thegraphicalelementsofadocumentorvisualpresentation.');
      });
      test('actual LaTeX', function() {
        assertPaste('a_nx^n+a_{n+1}x^{n+1}');
        assertPaste('\\frac{1}{2\\sqrt{x}}');
      });
      test('\\text{...}', function() {
        assertPaste('\\text{lol}');
        assertPaste('1+\\text{lol}+2');
        assertPaste('\\frac{\\text{apples}}{\\text{oranges}}');
      });
      test('selection', function(done) {
        mq.latex('x^2').select();
        setTimeout(function() {
          assert.equal(textarea.val(), 'x^2');
          done();
        });
      });
    });
    suite('statelessClipboard set to true', function() {
      var mq, textarea;
      setup(function() {
        mq = MQ.MathField($('<span></span>').appendTo('#mock')[0],
                                 { statelessClipboard: true });
        textarea = $(mq.el()).find('textarea');;
      });
      teardown(function() {
        $(mq.el()).remove();
      });
      function assertPaste(paste, latex) {
        if (arguments.length < 2) latex = paste;
        mq.latex('');
        textarea.trigger('paste').val(paste).trigger('input');
        assert.equal(mq.latex(), latex);
      }

      test('numbers and letters', function() {
        assertPaste('123xyz', '\\text{123xyz}');
      });
      test('a sentence', function() {
        assertPaste('Lorem ipsum is a placeholder text commonly used to '
                    + 'demonstrate the graphical elements of a document or '
                    + 'visual presentation.',
                    '\\text{Lorem ipsum is a placeholder text commonly used to '
                    + 'demonstrate the graphical elements of a document or '
                    + 'visual presentation.}');
      });
      test('backslashes', function() {
        assertPaste('something \\pi something \\asdf',
                    '\\text{something \\pi something \\asdf}');
      });
      // TODO: braces (currently broken)
      test('actual math LaTeX wrapped in dollar signs', function() {
        assertPaste('$a_nx^n+a_{n+1}x^{n+1}$', 'a_nx^n+a_{n+1}x^{n+1}');
        assertPaste('$\\frac{1}{2\\sqrt{x}}$', '\\frac{1}{2\\sqrt{x}}');
      });
      test('selection', function(done) {
        mq.latex('x^2').select();
        setTimeout(function() {
          assert.equal(textarea.val(), '$x^2$');
          done();
        });
      });
    });
  });

  suite('leftRightIntoCmdGoes: "up"/"down"', function() {
    test('"up" or "down" required', function() {
      assert.throws(function() {
        MQ.MathField($('<span></span>')[0], { leftRightIntoCmdGoes: 1 });
      });
    });
    suite('default', function() {
      var mq;
      setup(function() {
        mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
      });
      teardown(function() {
        $(mq.el()).remove();
      });

      test('fractions', function() {
        mq.latex('\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');
        assert.equal(mq.latex(), '\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.moveToLeftEnd().typedText('a');
        assert.equal(mq.latex(), 'a\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('b');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('c');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('d');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('e');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('f');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('g');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('h');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('i');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{i\\frac{3}{4}}');

        mq.keystroke('Right').typedText('j');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{i\\frac{j3}{4}}');

        mq.keystroke('Right Right').typedText('k');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{i\\frac{j3}{k4}}');

        mq.keystroke('Right Right').typedText('l');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{i\\frac{j3}{k4}l}');

        mq.keystroke('Right').typedText('m');
        assert.equal(mq.latex(), 'a\\frac{b1}{cx}d+\\frac{e\\frac{f1}{g2}h}{i\\frac{j3}{k4}l}m');
      });

      test('supsub', function() {
        mq.latex('x_a+y^b+z_a^b+w');
        assert.equal(mq.latex(), 'x_a+y^b+z_a^b+w');

        mq.moveToLeftEnd().typedText('1');
        assert.equal(mq.latex(), '1x_a+y^b+z_a^b+w');

        mq.keystroke('Right Right').typedText('2');
        assert.equal(mq.latex(), '1x_{2a}+y^b+z_a^b+w');

        mq.keystroke('Right Right').typedText('3');
        assert.equal(mq.latex(), '1x_{2a}3+y^b+z_a^b+w');

        mq.keystroke('Right Right Right').typedText('4');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}+z_a^b+w');

        mq.keystroke('Right Right').typedText('5');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_a^b+w');

        mq.keystroke('Right Right Right').typedText('6');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_{6a}^b+w');

        mq.keystroke('Right Right').typedText('7');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_{6a}^{7b}+w');

        mq.keystroke('Right Right').typedText('8');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_{6a}^{7b}8+w');
      });

      test('nthroot', function() {
        mq.latex('\\sqrt[n]{x}');
        assert.equal(mq.latex(), '\\sqrt[n]{x}');

        mq.moveToLeftEnd().typedText('1');
        assert.equal(mq.latex(), '1\\sqrt[n]{x}');

        mq.keystroke('Right').typedText('2');
        assert.equal(mq.latex(), '1\\sqrt[2n]{x}');

        mq.keystroke('Right Right').typedText('3');
        assert.equal(mq.latex(), '1\\sqrt[2n]{3x}');

        mq.keystroke('Right Right').typedText('4');
        assert.equal(mq.latex(), '1\\sqrt[2n]{3x}4');
      });
    });

    suite('"up"', function() {
      var mq;
      setup(function() {
        mq = MQ.MathField($('<span></span>').appendTo('#mock')[0],
                                 { leftRightIntoCmdGoes: 'up' });
      });
      teardown(function() {
        $(mq.el()).remove();
      });

      test('fractions', function() {
        mq.latex('\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');
        assert.equal(mq.latex(), '\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.moveToLeftEnd().typedText('a');
        assert.equal(mq.latex(), 'a\\frac{1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('b');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('c');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}c+\\frac{\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('d');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}c+\\frac{d\\frac{1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('e');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}c+\\frac{d\\frac{e1}{2}}{\\frac{3}{4}}');

        mq.keystroke('Right Right').typedText('f');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}c+\\frac{d\\frac{e1}{2}f}{\\frac{3}{4}}');

        mq.keystroke('Right').typedText('g');
        assert.equal(mq.latex(), 'a\\frac{b1}{x}c+\\frac{d\\frac{e1}{2}f}{\\frac{3}{4}}g');
      });

      test('supsub', function() {
        mq.latex('x_a+y^b+z_a^b+w');
        assert.equal(mq.latex(), 'x_a+y^b+z_a^b+w');

        mq.moveToLeftEnd().typedText('1');
        assert.equal(mq.latex(), '1x_a+y^b+z_a^b+w');

        mq.keystroke('Right Right').typedText('2');
        assert.equal(mq.latex(), '1x_{2a}+y^b+z_a^b+w');

        mq.keystroke('Right Right').typedText('3');
        assert.equal(mq.latex(), '1x_{2a}3+y^b+z_a^b+w');

        mq.keystroke('Right Right Right').typedText('4');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}+z_a^b+w');

        mq.keystroke('Right Right').typedText('5');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_a^b+w');

        mq.keystroke('Right Right Right').typedText('6');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_a^{6b}+w');

        mq.keystroke('Right Right').typedText('7');
        assert.equal(mq.latex(), '1x_{2a}3+y^{4b}5+z_a^{6b}7+w');
      });

      test('nthroot', function() {
        mq.latex('\\sqrt[n]{x}');
        assert.equal(mq.latex(), '\\sqrt[n]{x}');

        mq.moveToLeftEnd().typedText('1');
        assert.equal(mq.latex(), '1\\sqrt[n]{x}');

        mq.keystroke('Right').typedText('2');
        assert.equal(mq.latex(), '1\\sqrt[2n]{x}');

        mq.keystroke('Right Right').typedText('3');
        assert.equal(mq.latex(), '1\\sqrt[2n]{3x}');

        mq.keystroke('Right Right').typedText('4');
        assert.equal(mq.latex(), '1\\sqrt[2n]{3x}4');
      });
    });
  });

  suite('sumStartsWithNEquals', function() {
    test('sum defaults to empty limits', function() {
      var mq = MQ.MathField($('<span>').appendTo('#mock')[0]);
      assert.equal(mq.latex(), '');

      mq.cmd('\\sum');
      assert.equal(mq.latex(), '\\sum_{ }^{ }');

      mq.cmd('n');
      assert.equal(mq.latex(), '\\sum_n^{ }', 'cursor in lower limit');

      $(mq.el()).remove();
    });
    test('sum starts with `n=`', function() {
      var mq = MQ.MathField($('<span>').appendTo('#mock')[0], {
        sumStartsWithNEquals: true
      });
      assert.equal(mq.latex(), '');

      mq.cmd('\\sum');
      assert.equal(mq.latex(), '\\sum_{n=}^{ }');

      mq.cmd('0');
      assert.equal(mq.latex(), '\\sum_{n=0}^{ }', 'cursor after the `n=`');

      $(mq.el()).remove();
    });
  });

  suite('substituteTextarea', function() {
    test('doesn\'t blow up on selection', function() {
      var mq = MQ.MathField($('<span>').appendTo('#mock')[0], {
        substituteTextarea: function() {
          return $('<span tabindex=0 style="display:inline-block;width:1px;height:1px" />')[0];
        }
      });

      assert.equal(mq.latex(), '');
      mq.write('asdf');
      mq.select();

      $(mq.el()).remove();
    });
  });

  suite('dropEmbedded', function() {
    test('inserts into empty', function() {
      var mq = MQ.MathField($('<span>').appendTo('#mock')[0]);
      mq.dropEmbedded(0, 0, {
        htmlString: '<span class="embedded-html"></span>',
        text: function () { return "embedded text" },
        latex: function () { return "embedded latex" }
      });

      assert.ok(jQuery('.embedded-html').length);
      assert.equal(mq.text(), "embedded text");
      assert.equal(mq.latex(), "embedded latex");

      $(mq.el()).remove();
    });
    test('inserts at coordinates', function() {
      // Insert filler so that the page is taller than the window so this test is deterministic
      // Test that we use clientY instead of pageY
      var windowHeight = $(window).height();
      var filler = $('<div>').height(windowHeight);
      filler.insertBefore('#mock');

      var mq = MQ.MathField($('<span>').appendTo('#mock')[0]);
      mq.typedText("mmmm/mmmm");
      var pos = $(mq.el()).offset();
      var mqx = pos.left;
      var mqy = pos.top;

      mq.el().scrollIntoView();

      mq.dropEmbedded(mqx + 30, mqy + 40, {
        htmlString: '<span class="embedded-html"></span>',
        text: function () { return "embedded text" },
        latex: function () { return "embedded latex" }
      });

      assert.ok(jQuery('.embedded-html').length);
      assert.equal(mq.text(), "(m*m*m*m)/(m*m*embedded text*m*m)");
      assert.equal(mq.latex(), "\\frac{mmmm}{mmembedded latexmm}");

      filler.remove();
      $(mq.el()).remove();
    });
  });

  test('.registerEmbed()', function() {
    var calls = 0, data;
    MQ.registerEmbed('thing', function(data_) {
      calls += 1;
      data = data_;
      return {
        htmlString: '<span class="embedded-html"></span>',
        text: function () { return "embedded text" },
        latex: function () { return "embedded latex" }
      };
    });
    var mq = MQ.MathField($('<span>\\sqrt{\\embed{thing}}</span>').appendTo('#mock')[0]);
    assert.equal(calls, 1);
    assert.equal(data, undefined);

    assert.ok(jQuery('.embedded-html').length);
    assert.equal(mq.text(), "sqrt(embedded text)");
    assert.equal(mq.latex(), "\\sqrt{embedded latex}");

    mq.latex('\\sqrt{\\embed{thing}[data]}');
    assert.equal(calls, 2);
    assert.equal(data, 'data');

    assert.ok(jQuery('.embedded-html').length);
    assert.equal(mq.text(), "sqrt(embedded text)");
    assert.equal(mq.latex(), "\\sqrt{embedded latex}");

    $(mq.el()).remove();
  });
});
suite('saneKeyboardEvents', function() {
  var el;
  var Event = jQuery.Event

  function supportsSelectionAPI() {
    return 'selectionStart' in el[0];
  }

  setup(function() {
    el = $('<textarea>').appendTo('#mock');
  });

  teardown(function() {
    el.remove();
  });

  test('normal keys', function(done) {
    var counter = 0;
    saneKeyboardEvents(el, {
      keystroke: noop,
      typedText: function(text, keydown, keypress) {
        counter += 1;
        assert.ok(counter <= 1, 'callback is only called once');
        assert.equal(text, 'a', 'text comes back as a');
        assert.equal(el.val(), '', 'the textarea remains empty');

        done();
      }
    });

    el.trigger(Event('keydown', { which: 97 }));
    el.trigger(Event('keypress', { which: 97 }));
    el.val('a');
  });

  test('one keydown only', function(done) {
    var counter = 0;

    saneKeyboardEvents(el, {
      keystroke: function(key, evt) {
        counter += 1;
        assert.ok(counter <= 1, 'callback is called only once');
        assert.equal(key, 'Backspace', 'key is correctly set');

        done();
      }
    });

    el.trigger(Event('keydown', { which: 8 }));
  });

  test('a series of keydowns only', function(done) {
    var counter = 0;

    saneKeyboardEvents(el, {
      keystroke: function(key, keydown) {
        counter += 1;
        assert.ok(counter <= 3, 'callback is called at most 3 times');

        assert.ok(keydown);
        assert.equal(key, 'Left');

        if (counter === 3) done();
      }
    });

    el.trigger(Event('keydown', { which: 37 }));
    el.trigger(Event('keydown', { which: 37 }));
    el.trigger(Event('keydown', { which: 37 }));
  });

  test('one keydown and a series of keypresses', function(done) {
    var counter = 0;

    saneKeyboardEvents(el, {
      keystroke: function(key, keydown) {
        counter += 1;
        assert.ok(counter <= 3, 'callback is called at most 3 times');

        assert.ok(keydown);
        assert.equal(key, 'Backspace');

        if (counter === 3) done();
      }
    });

    el.trigger(Event('keydown', { which: 8 }));
    el.trigger(Event('keypress', { which: 8 }));
    el.trigger(Event('keypress', { which: 8 }));
    el.trigger(Event('keypress', { which: 8 }));
  });

  suite('select', function() {
    test('select populates the textarea but doesn\'t call .typedText()', function() {
      var shim = saneKeyboardEvents(el, { keystroke: noop });

      shim.select('foobar');

      assert.equal(el.val(), 'foobar');
      el.trigger('keydown');
      assert.equal(el.val(), 'foobar', 'value remains after keydown');

      if (supportsSelectionAPI()) {
        el.trigger('keypress');
        assert.equal(el.val(), 'foobar', 'value remains after keypress');
        el.trigger('input');
        assert.equal(el.val(), 'foobar', 'value remains after flush after keypress');
      }
    });

    test('select populates the textarea but doesn\'t call text' +
         ' on keydown, even when the selection is not properly' +
         ' detectable', function() {
      var shim = saneKeyboardEvents(el, { keystroke: noop });

      shim.select('foobar');
      // monkey-patch the dom-level selection so that hasSelection()
      // returns false, as in IE < 9.
      el[0].selectionStart = el[0].selectionEnd = 0;

      el.trigger('keydown');
      assert.equal(el.val(), 'foobar', 'value remains after keydown');
    });

    test('blurring', function() {
      var shim = saneKeyboardEvents(el, { keystroke: noop });

      shim.select('foobar');
      el.trigger('blur');
      el.focus();

      // IE < 9 doesn't support selection{Start,End}
      if (supportsSelectionAPI()) {
        assert.equal(el[0].selectionStart, 0, 'it\'s selected from the start');
        assert.equal(el[0].selectionEnd, 6, 'it\'s selected to the end');
      }

      assert.equal(el.val(), 'foobar', 'it still has content');
    });

    test('blur then empty selection', function() {
      var shim = saneKeyboardEvents(el, { keystroke: noop });
      shim.select('foobar');
      el.blur();
      shim.select('');
      assert.ok(document.activeElement !== el[0], 'textarea remains blurred');
    });

    if (!document.hasFocus()) {
      test('blur in keystroke handler: DOCUMENT NEEDS FOCUS, SEE CONSOLE ');
      console.warn(
        'The test "blur in keystroke handler" needs the document to have ' +
        'focus. Only when the document has focus does .select() on an ' +
        'element also focus it, which is part of the problematic behavior ' +
        'we are testing robustness against. (Specifically, erroneously ' +
        'calling .select() in a timeout after the textarea has blurred, ' +
        '"stealing back" focus.)\n' +
        'Normally, the page being open and focused is enough to have focus, ' +
        'but with the Developer Tools open, it depends on whether you last ' +
        'clicked on something in the Developer Tools or on the page itself. ' +
        'Click the page, or close the Developer Tools, and Refresh.'
      );
    }
    else {
      test('blur in keystroke handler', function(done) {
        var shim = saneKeyboardEvents(el, {
          keystroke: function(key) {
            assert.equal(key, 'Left');
            el[0].blur();
          }
        });

        shim.select('foobar');
        assert.ok(document.activeElement === el[0], 'textarea focused');

        el.trigger(Event('keydown', { which: 37 }));
        assert.ok(document.activeElement !== el[0], 'textarea blurred');

        setTimeout(function() {
          assert.ok(document.activeElement !== el[0], 'textarea remains blurred');
          done();
        });
      });
    }

    suite('selected text after keypress or paste doesn\'t get mistaken' +
         ' for inputted text', function() {
      test('select() immediately after paste', function() {
        var pastedText;
        var onPaste = function(text) { pastedText = text; };
        var shim = saneKeyboardEvents(el, {
          paste: function(text) { onPaste(text); }
        });

        el.trigger('paste').val('$x^2+1$');

        shim.select('$\\frac{x^2+1}{2}$');
        assert.equal(pastedText, '$x^2+1$');
        assert.equal(el.val(), '$\\frac{x^2+1}{2}$');

        onPaste = null;

        shim.select('$2$');
        assert.equal(el.val(), '$2$');
      });

      test('select() after paste/input', function() {
        var pastedText;
        var onPaste = function(text) { pastedText = text; };
        var shim = saneKeyboardEvents(el, {
          paste: function(text) { onPaste(text); }
        });

        el.trigger('paste').val('$x^2+1$');

        el.trigger('input');
        assert.equal(pastedText, '$x^2+1$');
        assert.equal(el.val(), '');

        onPaste = null;

        shim.select('$\\frac{x^2+1}{2}$');
        assert.equal(el.val(), '$\\frac{x^2+1}{2}$');

        shim.select('$2$');
        assert.equal(el.val(), '$2$');
      });

      test('select() immediately after keydown/keypress', function() {
        var typedText;
        var onText = function(text) { typedText = text; };
        var shim = saneKeyboardEvents(el, {
          keystroke: noop,
          typedText: function(text) { onText(text); }
        });

        el.trigger(Event('keydown', { which: 97 }));
        el.trigger(Event('keypress', { which: 97 }));
        el.val('a');

        shim.select('$\\frac{a}{2}$');
        assert.equal(typedText, 'a');
        assert.equal(el.val(), '$\\frac{a}{2}$');

        onText = null;

        shim.select('$2$');
        assert.equal(el.val(), '$2$');
      });

      test('select() after keydown/keypress/input', function() {
        var typedText;
        var onText = function(text) { typedText = text; };
        var shim = saneKeyboardEvents(el, {
          keystroke: noop,
          typedText: function(text) { onText(text); }
        });

        el.trigger(Event('keydown', { which: 97 }));
        el.trigger(Event('keypress', { which: 97 }));
        el.val('a');

        el.trigger('input');
        assert.equal(typedText, 'a');

        onText = null;

        shim.select('$\\frac{a}{2}$');
        assert.equal(el.val(), '$\\frac{a}{2}$');

        shim.select('$2$');
        assert.equal(el.val(), '$2$');
      });

      suite('unrecognized keys that move cursor and clear selection', function() {
        test('without keypress', function() {
          var shim = saneKeyboardEvents(el, { keystroke: noop });

          shim.select('a');
          assert.equal(el.val(), 'a');

          if (!supportsSelectionAPI()) return;

          el.trigger(Event('keydown', { which: 37, altKey: true }));
          el[0].selectionEnd = 0;
          el.trigger(Event('keyup', { which: 37, altKey: true }));
          assert.ok(el[0].selectionStart !== el[0].selectionEnd);

          el.blur();
          shim.select('');
          assert.ok(document.activeElement !== el[0], 'textarea remains blurred');
        });

        test('with keypress, many characters selected', function() {
          var shim = saneKeyboardEvents(el, { keystroke: noop });

          shim.select('many characters');
          assert.equal(el.val(), 'many characters');

          if (!supportsSelectionAPI()) return;

          el.trigger(Event('keydown', { which: 37, altKey: true }));
          el.trigger(Event('keypress', { which: 37, altKey: true }));
          el[0].selectionEnd = 0;

          el.trigger('keyup');
          assert.ok(el[0].selectionStart !== el[0].selectionEnd);

          el.blur();
          shim.select('');
          assert.ok(document.activeElement !== el[0], 'textarea remains blurred');
        });

        test('with keypress, only 1 character selected', function() {
          var count = 0;
          var shim = saneKeyboardEvents(el, {
            keystroke: noop,
            typedText: function(ch) {
              assert.equal(ch, 'a');
              assert.equal(el.val(), '');
              count += 1;
            }
          });

          shim.select('a');
          assert.equal(el.val(), 'a');

          if (!supportsSelectionAPI()) return;

          el.trigger(Event('keydown', { which: 37, altKey: true }));
          el.trigger(Event('keypress', { which: 37, altKey: true }));
          el[0].selectionEnd = 0;

          el.trigger('keyup');
          assert.equal(count, 1);

          el.blur();
          shim.select('');
          assert.ok(document.activeElement !== el[0], 'textarea remains blurred');
        });
      });
    });
  });

  suite('paste', function() {
    test('paste event only', function(done) {
      saneKeyboardEvents(el, {
        paste: function(text) {
          assert.equal(text, '$x^2+1$');

          done();
        }
      });

      el.trigger('paste');
      el.val('$x^2+1$');
    });

    test('paste after keydown/keypress', function(done) {
      saneKeyboardEvents(el, {
        keystroke: noop,
        paste: function(text) {
          assert.equal(text, 'foobar');
          done();
        }
      });

      // Ctrl-V in Firefox or Opera, according to unixpapa.com/js/key.html
      // without an `input` event
      el.trigger('keydown', { which: 86, ctrlKey: true });
      el.trigger('keypress', { which: 118, ctrlKey: true });
      el.trigger('paste');
      el.val('foobar');
    });

    test('paste after keydown/keypress/input', function(done) {
      saneKeyboardEvents(el, {
        keystroke: noop,
        paste: function(text) {
          assert.equal(text, 'foobar');
          done();
        }
      });

      // Ctrl-V in Firefox or Opera, according to unixpapa.com/js/key.html
      // with an `input` event
      el.trigger('keydown', { which: 86, ctrlKey: true });
      el.trigger('keypress', { which: 118, ctrlKey: true });
      el.trigger('paste');
      el.val('foobar');
      el.trigger('input');
    });

    test('keypress timeout happening before paste timeout', function(done) {
      saneKeyboardEvents(el, {
        keystroke: noop,
        paste: function(text) {
          assert.equal(text, 'foobar');
          done();
        }
      });

      el.trigger('keydown', { which: 86, ctrlKey: true });
      el.trigger('keypress', { which: 118, ctrlKey: true });
      el.trigger('paste');
      el.val('foobar');

      // this synthesizes the keypress timeout calling handleText()
      // before the paste timeout happens.
      el.trigger('input');
    });
  });
});
suite('Cursor::select()', function() {
  var cursor = Cursor();
  cursor.selectionChanged = noop;

  function assertSelection(A, B, leftEnd, rightEnd) {
    var lca = leftEnd.parent, frag = Fragment(leftEnd, rightEnd || leftEnd);

    (function eitherOrder(A, B) {

      var count = 0;
      lca.selectChildren = function(leftEnd, rightEnd) {
        count += 1;
        assert.equal(frag.ends[L], leftEnd);
        assert.equal(frag.ends[R], rightEnd);
        return Node.p.selectChildren.apply(this, arguments);
      };

      Point.p.init.call(cursor, A.parent, A[L], A[R]);
      cursor.startSelection();
      Point.p.init.call(cursor, B.parent, B[L], B[R]);
      assert.equal(cursor.select(), true);
      assert.equal(count, 1);

      return eitherOrder;
    }(A, B)(B, A));
  }

  var parent = Node();
  var child1 = Node().adopt(parent, parent.ends[R], 0);
  var child2 = Node().adopt(parent, parent.ends[R], 0);
  var child3 = Node().adopt(parent, parent.ends[R], 0);
  var A = Point(parent, 0, child1);
  var B = Point(parent, child1, child2);
  var C = Point(parent, child2, child3);
  var D = Point(parent, child3, 0);
  var pt1 = Point(child1, 0, 0);
  var pt2 = Point(child2, 0, 0);
  var pt3 = Point(child3, 0, 0);

  test('same parent, one Node', function() {
    assertSelection(A, B, child1);
    assertSelection(B, C, child2);
    assertSelection(C, D, child3);
  });

  test('same Parent, many Nodes', function() {
    assertSelection(A, C, child1, child2);
    assertSelection(A, D, child1, child3);
    assertSelection(B, D, child2, child3);
  });

  test('Point next to parent of other Point', function() {
    assertSelection(A, pt1, child1);
    assertSelection(B, pt1, child1);

    assertSelection(B, pt2, child2);
    assertSelection(C, pt2, child2);

    assertSelection(C, pt3, child3);
    assertSelection(D, pt3, child3);
  });

  test('Points\' parents are siblings', function() {
    assertSelection(pt1, pt2, child1, child2);
    assertSelection(pt2, pt3, child2, child3);
    assertSelection(pt1, pt3, child1, child3);
  });

  test('Point is sibling of parent of other Point', function() {
    assertSelection(A, pt2, child1, child2);
    assertSelection(A, pt3, child1, child3);
    assertSelection(B, pt3, child2, child3);
    assertSelection(pt1, D, child1, child3);
    assertSelection(pt1, C, child1, child2);
  });

  test('same Point', function() {
    Point.p.init.call(cursor, A.parent, A[L], A[R]);
    cursor.startSelection();
    assert.equal(cursor.select(), false);
  });

  test('different trees', function() {
    var anotherTree = Node();

    Point.p.init.call(cursor, A.parent, A[L], A[R]);
    cursor.startSelection();
    Point.p.init.call(cursor, anotherTree, 0, 0);
    assert.throws(function() { cursor.select(); });

    Point.p.init.call(cursor, anotherTree, 0, 0);
    cursor.startSelection();
    Point.p.init.call(cursor, A.parent, A[L], A[R]);
    assert.throws(function() { cursor.select(); });
  });
});
suite('text', function() {

  function fromLatex(latex) {
    var block = latexMathParser.parse(latex);
    block.jQize();

    return block;
  }

  function assertSplit(jQ, prev, next) {
    var dom = jQ[0];

    if (prev) {
      assert.ok(dom.previousSibling instanceof Text);
      assert.equal(prev, dom.previousSibling.data);
    }
    else {
      assert.ok(!dom.previousSibling);
    }

    if (next) {
      assert.ok(dom.nextSibling instanceof Text);
      assert.equal(next, dom.nextSibling.data);
    }
    else {
      assert.ok(!dom.nextSibling);
    }
  }

  test('changes the text nodes as the cursor moves around', function() {
    var block = fromLatex('\\text{abc}');
    var ctrlr = Controller(block, 0, 0);
    var cursor = ctrlr.cursor.insAtRightEnd(block);

    ctrlr.moveLeft();
    assertSplit(cursor.jQ, 'abc', null);

    ctrlr.moveLeft();
    assertSplit(cursor.jQ, 'ab', 'c');

    ctrlr.moveLeft();
    assertSplit(cursor.jQ, 'a', 'bc');

    ctrlr.moveLeft();
    assertSplit(cursor.jQ, null, 'abc');

    ctrlr.moveRight();
    assertSplit(cursor.jQ, 'a', 'bc');

    ctrlr.moveRight();
    assertSplit(cursor.jQ, 'ab', 'c');

    ctrlr.moveRight();
    assertSplit(cursor.jQ, 'abc', null);
  });

  test('does not change latex as the cursor moves around', function() {
    var block = fromLatex('\\text{x}');
    var ctrlr = Controller(block, 0, 0);
    var cursor = ctrlr.cursor.insAtRightEnd(block);

    ctrlr.moveLeft();
    ctrlr.moveLeft();
    ctrlr.moveLeft();

    assert.equal(block.latex(), '\\text{x}');
  });
});
suite('tree', function() {
  suite('adopt', function() {
    function assertTwoChildren(parent, one, two) {
      assert.equal(one.parent, parent, 'one.parent is set');
      assert.equal(two.parent, parent, 'two.parent is set');

      assert.ok(!one[L], 'one has nothing leftward');
      assert.equal(one[R], two, 'one[R] is two');
      assert.equal(two[L], one, 'two[L] is one');
      assert.ok(!two[R], 'two has nothing rightward');

      assert.equal(parent.ends[L], one, 'parent.ends[L] is one');
      assert.equal(parent.ends[R], two, 'parent.ends[R] is two');
    }

    test('the empty case', function() {
      var parent = Node();
      var child = Node();

      child.adopt(parent, 0, 0);

      assert.equal(child.parent, parent, 'child.parent is set');
      assert.ok(!child[R], 'child has nothing rightward');
      assert.ok(!child[L], 'child has nothing leftward');

      assert.equal(parent.ends[L], child, 'child is parent.ends[L]');
      assert.equal(parent.ends[R], child, 'child is parent.ends[R]');
    });

    test('with two children from the left', function() {
      var parent = Node();
      var one = Node();
      var two = Node();

      one.adopt(parent, 0, 0);
      two.adopt(parent, one, 0);

      assertTwoChildren(parent, one, two);
    });

    test('with two children from the right', function() {
      var parent = Node();
      var one = Node();
      var two = Node();

      two.adopt(parent, 0, 0);
      one.adopt(parent, 0, two);

      assertTwoChildren(parent, one, two);
    });

    test('adding one in the middle', function() {
      var parent = Node();
      var leftward = Node();
      var rightward = Node();
      var middle = Node();

      leftward.adopt(parent, 0, 0);
      rightward.adopt(parent, leftward, 0);
      middle.adopt(parent, leftward, rightward);

      assert.equal(middle.parent, parent, 'middle.parent is set');
      assert.equal(middle[L], leftward, 'middle[L] is set');
      assert.equal(middle[R], rightward, 'middle[R] is set');

      assert.equal(leftward[R], middle, 'leftward[R] is middle');
      assert.equal(rightward[L], middle, 'rightward[L] is middle');

      assert.equal(parent.ends[L], leftward, 'parent.ends[L] is leftward');
      assert.equal(parent.ends[R], rightward, 'parent.ends[R] is rightward');
    });
  });

  suite('disown', function() {
    function assertSingleChild(parent, child) {
      assert.equal(parent.ends[L], child, 'parent.ends[L] is child');
      assert.equal(parent.ends[R], child, 'parent.ends[R] is child');
      assert.ok(!child[L], 'child has nothing leftward');
      assert.ok(!child[R], 'child has nothing rightward');
    }

    test('the empty case', function() {
      var parent = Node();
      var child = Node();

      child.adopt(parent, 0, 0);
      child.disown();

      assert.ok(!parent.ends[L], 'parent has no left end child');
      assert.ok(!parent.ends[R], 'parent has no right end child');
    });

    test('disowning the right end child', function() {
      var parent = Node();
      var one = Node();
      var two = Node();

      one.adopt(parent, 0, 0);
      two.adopt(parent, one, 0);

      two.disown();

      assertSingleChild(parent, one);

      assert.equal(two.parent, parent, 'two retains its parent');
      assert.equal(two[L], one, 'two retains its [L]');

      assert.throws(function() { two.disown(); },
                    'disown fails on a malformed tree');
    });

    test('disowning the left end child', function() {
      var parent = Node();
      var one = Node();
      var two = Node();

      one.adopt(parent, 0, 0);
      two.adopt(parent, one, 0);

      one.disown();

      assertSingleChild(parent, two);

      assert.equal(one.parent, parent, 'one retains its parent');
      assert.equal(one[R], two, 'one retains its [R]');

      assert.throws(function() { one.disown(); },
                    'disown fails on a malformed tree');
    });

    test('disowning the middle', function() {
      var parent = Node();
      var leftward = Node();
      var rightward = Node();
      var middle = Node();

      leftward.adopt(parent, 0, 0);
      rightward.adopt(parent, leftward, 0);
      middle.adopt(parent, leftward, rightward);

      middle.disown();

      assert.equal(leftward[R], rightward, 'leftward[R] is rightward');
      assert.equal(rightward[L], leftward, 'rightward[L] is leftward');
      assert.equal(parent.ends[L], leftward, 'parent.ends[L] is leftward');
      assert.equal(parent.ends[R], rightward, 'parent.ends[R] is rightward');

      assert.equal(middle.parent, parent, 'middle retains its parent');
      assert.equal(middle[R], rightward, 'middle retains its [R]');
      assert.equal(middle[L], leftward, 'middle retains its [L]');

      assert.throws(function() { middle.disown(); },
                    'disown fails on a malformed tree');
    });
  });

  suite('fragments', function() {
    test('an empty fragment', function() {
      var empty = Fragment();
      var count = 0;

      empty.each(function() { count += 1 });

      assert.equal(count, 0, 'each is a noop on an empty fragment');
    });

    test('half-empty fragments are disallowed', function() {
      assert.throws(function() {
        Fragment(Node(), 0)
      }, 'half-empty on the right');

      assert.throws(function() {
        Fragment(0, Node());
      }, 'half-empty on the left');
    });

    test('directionalized constructor call', function() {
      var ChNode = P(Node, { init: function(ch) { this.ch = ch; } });
      var parent = Node();
      var a = ChNode('a').adopt(parent, parent.ends[R], 0);
      var b = ChNode('b').adopt(parent, parent.ends[R], 0);
      var c = ChNode('c').adopt(parent, parent.ends[R], 0);
      var d = ChNode('d').adopt(parent, parent.ends[R], 0);
      var e = ChNode('e').adopt(parent, parent.ends[R], 0);

      function cat(str, node) { return str + node.ch; }
      assert.equal('bcd', Fragment(b, d).fold('', cat));
      assert.equal('bcd', Fragment(b, d, L).fold('', cat));
      assert.equal('bcd', Fragment(d, b, R).fold('', cat));
      assert.throws(function() { Fragment(d, b, L); });
      assert.throws(function() { Fragment(b, d, R); });
    });

    test('disown is idempotent', function() {
      var parent = Node();
      var one = Node().adopt(parent, 0, 0);
      var two = Node().adopt(parent, one, 0);

      var frag = Fragment(one, two);
      frag.disown();
      frag.disown();
    });
  });
});
suite('typing with auto-replaces', function() {
  var mq;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  function prayWellFormedPoint(pt) { prayWellFormed(pt.parent, pt[L], pt[R]); }
  function assertLatex(latex) {
    prayWellFormedPoint(mq.__controller.cursor);
    assert.equal(mq.latex(), latex);
  }

  suite('LiveFraction', function() {
    test('full MathQuill', function() {
      mq.typedText('1/2').keystroke('Tab').typedText('+sinx/');
      assertLatex('\\frac{1}{2}+\\frac{\\sin x}{ }');
      mq.latex('').typedText('1+/2');
      assertLatex('1+\\frac{2}{ }');
      mq.latex('').typedText('1 2/3');
      assertLatex('1\\ \\frac{2}{3}');
    });

    test('mathquill-basic', function() {
      var mq_basic = MQBasic.MathField($('<span></span>').appendTo('#mock')[0]);
      mq_basic.typedText('1/2');
      assert.equal(mq_basic.latex(), '\\frac{1}{2}');
      $(mq_basic.el()).remove();
    });
  });

  suite('LatexCommandInput', function() {
    test('basic', function() {
      mq.typedText('\\sqrt-x');
      assertLatex('\\sqrt{-x}');
    });

    test('they\'re passed their name', function() {
      mq.cmd('\\alpha');
      assert.equal(mq.latex(), '\\alpha');
    });

    test('replaces selection', function() {
      mq.typedText('49').select().typedText('\\sqrt').keystroke('Enter');
      assertLatex('\\sqrt{49}');
    });

    test('auto-operator names', function() {
      mq.typedText('\\sin^2');
      assertLatex('\\sin^2');
    });

    test('nonexistent LaTeX command', function() {
      mq.typedText('\\asdf+');
      assertLatex('\\text{asdf}+');
    });
  });

  suite('auto-expanding parens', function() {
    suite('simple', function() {
      test('empty parens ()', function() {
        mq.typedText('(');
        assertLatex('\\left(\\right)');
        mq.typedText(')');
        assertLatex('\\left(\\right)');
      });

      test('straight typing 1+(2+3)+4', function() {
        mq.typedText('1+(2+3)+4');
        assertLatex('1+\\left(2+3\\right)+4');
      });

      test('basic command \\sin(', function () {
        mq.typedText('\\sin(');
        assertLatex('\\sin\\left(\\right)');
      });

      test('wrapping things in parens 1+(2+3)+4', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Left Left').typedText(')');
        assertLatex('\\left(1+2+3\\right)+4');
        mq.keystroke('Left Left Left Left').typedText('(');
        assertLatex('1+\\left(2+3\\right)+4');
      });

      test('nested parens 1+(2+(3+4)+5)+6', function() {
        mq.typedText('1+(2+(3+4)+5)+6');
        assertLatex('1+\\left(2+\\left(3+4\\right)+5\\right)+6');
      });
    });

    suite('mismatched brackets', function() {
      test('empty mismatched brackets (] and [}', function() {
        mq.typedText('(');
        assertLatex('\\left(\\right)');
        mq.typedText(']');
        assertLatex('\\left(\\right]');
        mq.typedText('[');
        assertLatex('\\left(\\right]\\left[\\right]');
        mq.typedText('}');
        assertLatex('\\left(\\right]\\left[\\right\\}');
      });

      test('typing mismatched brackets 1+(2+3]+4', function() {
        mq.typedText('1+');
        assertLatex('1+');
        mq.typedText('(');
        assertLatex('1+\\left(\\right)');
        mq.typedText('2+3');
        assertLatex('1+\\left(2+3\\right)');
        mq.typedText(']+4');
        assertLatex('1+\\left(2+3\\right]+4');
      });

      test('wrapping things in mismatched brackets 1+(2+3]+4', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Left Left').typedText(']');
        assertLatex('\\left[1+2+3\\right]+4');
        mq.keystroke('Left Left Left Left').typedText('(');
        assertLatex('1+\\left(2+3\\right]+4');
      });

      test('nested mismatched brackets 1+(2+[3+4)+5]+6', function() {
        mq.typedText('1+(2+[3+4)+5]+6');
        assertLatex('1+\\left(2+\\left[3+4\\right)+5\\right]+6');
      });

      suite('restrictMismatchedBrackets', function() {
        setup(function() {
          mq.config({ restrictMismatchedBrackets: true });
        });
        test('typing (|x|+1) works', function() {
          mq.typedText('(|x|+1)');
          assertLatex('\\left(\\left|x\\right|+1\\right)');
        });
        test('typing [x} becomes [{x}]', function() {
          mq.typedText('[x}');
          assertLatex('\\left[\\left\\{x\\right\\}\\right]');
        });
        test('normal matching pairs {f(n), [a,b]} work', function() {
          mq.typedText('{f(n), [a,b]}');
          assertLatex('\\left\\{f\\left(n\\right),\\ \\left[a,b\\right]\\right\\}');
        });
        test('[a,b) and (a,b] still work', function() {
          mq.typedText('[a,b) + (a,b]');
          assertLatex('\\left[a,b\\right)\\ +\\ \\left(a,b\\right]');
        });
      });
    });

    suite('pipes', function() {
      test('empty pipes ||', function() {
        mq.typedText('|');
        assertLatex('\\left|\\right|');
        mq.typedText('|');
        assertLatex('\\left|\\right|');
      });

      test('straight typing 1+|2+3|+4', function() {
        mq.typedText('1+|2+3|+4');
        assertLatex('1+\\left|2+3\\right|+4');
      });

      test('wrapping things in pipes 1+|2+3|+4', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Home Right Right').typedText('|');
        assertLatex('1+\\left|2+3+4\\right|');
        mq.keystroke('Right Right Right').typedText('|');
        assertLatex('1+\\left|2+3\\right|+4');
      });

      suite('can type mismatched paren/pipe group from any side', function() {
        suite('straight typing', function() {
          test('|)', function() {
            mq.typedText('|)');
            assertLatex('\\left|\\right)');
          });

          test('(|', function() {
            mq.typedText('(|');
            assertLatex('\\left(\\right|');
          });
        });

        suite('the other direction', function() {
          test('|)', function() {
            mq.typedText(')');
            assertLatex('\\left(\\right)');
            mq.keystroke('Left').typedText('|');
            assertLatex('\\left|\\right)');
          });

          test('(|', function() {
            mq.typedText('||');
            assertLatex('\\left|\\right|');
            mq.keystroke('Left Left Del');
            assertLatex('\\left|\\right|');
            mq.typedText('(');
            assertLatex('\\left(\\right|');
          });
        });
      });
    });

    suite('backspacing', backspacingTests);

    suite('backspacing with restrictMismatchedBrackets', function() {
      setup(function() {
        mq.config({ restrictMismatchedBrackets: true });
      });

      backspacingTests();
    });

    function backspacingTests() {
      test('typing then backspacing a close-paren in the middle of 1+2+3+4', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Left Left').typedText(')');
        assertLatex('\\left(1+2+3\\right)+4');
        mq.keystroke('Backspace');
        assertLatex('1+2+3+4');
      });

      test('backspacing close-paren then open-paren of 1+(2+3)+4', function() {
        mq.typedText('1+(2+3)+4');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('1+\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('backspacing open-paren of 1+(2+3)+4', function() {
        mq.typedText('1+(2+3)+4');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Left Left Left Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('backspacing close-bracket then open-paren of 1+(2+3]+4', function() {
        mq.typedText('1+(2+3]+4');
        assertLatex('1+\\left(2+3\\right]+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('1+\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('backspacing open-paren of 1+(2+3]+4', function() {
        mq.typedText('1+(2+3]+4');
        assertLatex('1+\\left(2+3\\right]+4');
        mq.keystroke('Left Left Left Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });


      test('backspacing close-bracket then open-paren of 1+(2+3] (nothing after paren group)', function() {
        mq.typedText('1+(2+3]');
        assertLatex('1+\\left(2+3\\right]');
        mq.keystroke('Backspace');
        assertLatex('1+\\left(2+3\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3');
      });

      test('backspacing open-paren of 1+(2+3] (nothing after paren group)', function() {
        mq.typedText('1+(2+3]');
        assertLatex('1+\\left(2+3\\right]');
        mq.keystroke('Left Left Left Left Backspace');
        assertLatex('1+2+3');
      });

      test('backspacing close-bracket then open-paren of (2+3]+4 (nothing before paren group)', function() {
        mq.typedText('(2+3]+4');
        assertLatex('\\left(2+3\\right]+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('2+3+4');
      });

      test('backspacing open-paren of (2+3]+4 (nothing before paren group)', function() {
        mq.typedText('(2+3]+4');
        assertLatex('\\left(2+3\\right]+4');
        mq.keystroke('Left Left Left Left Left Left Backspace');
        assertLatex('2+3+4');
      });

      function assertParenBlockNonEmpty() {
        var parenBlock = $(mq.el()).find('.mq-paren+span');
        assert.equal(parenBlock.length, 1, 'exactly 1 paren block');
        assert.ok(!parenBlock.hasClass('mq-empty'),
                  'paren block auto-expanded, should no longer be gray');
      }

      test('backspacing close-bracket then open-paren of 1+(]+4 (empty paren group)', function() {
        mq.typedText('1+(]+4');
        assertLatex('1+\\left(\\right]+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('1+\\left(+4\\right)');
        assertParenBlockNonEmpty();
        mq.keystroke('Backspace');
        assertLatex('1++4');
      });

      test('backspacing open-paren of 1+(]+4 (empty paren group)', function() {
        mq.typedText('1+(]+4');
        assertLatex('1+\\left(\\right]+4');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1++4');
      });

      test('backspacing close-bracket then open-paren of 1+(] (empty paren group, nothing after)', function() {
        mq.typedText('1+(]');
        assertLatex('1+\\left(\\right]');
        mq.keystroke('Backspace');
        assertLatex('1+\\left(\\right)');
        mq.keystroke('Backspace');
        assertLatex('1+');
      });

      test('backspacing open-paren of 1+(] (empty paren group, nothing after)', function() {
        mq.typedText('1+(]');
        assertLatex('1+\\left(\\right]');
        mq.keystroke('Left Backspace');
        assertLatex('1+');
      });

      test('backspacing close-bracket then open-paren of (]+4 (empty paren group, nothing before)', function() {
        mq.typedText('(]+4');
        assertLatex('\\left(\\right]+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('\\left(+4\\right)');
        assertParenBlockNonEmpty();
        mq.keystroke('Backspace');
        assertLatex('+4');
      });

      test('backspacing open-paren of (]+4 (empty paren group, nothing before)', function() {
        mq.typedText('(]+4');
        assertLatex('\\left(\\right]+4');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('+4');
      });

      test('rendering mismatched brackets 1+(2+3]+4 from LaTeX then backspacing close-bracket then open-paren', function() {
        mq.latex('1+\\left(2+3\\right]+4');
        assertLatex('1+\\left(2+3\\right]+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('1+\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('rendering mismatched brackets 1+(2+3]+4 from LaTeX then backspacing open-paren', function() {
        mq.latex('1+\\left(2+3\\right]+4');
        assertLatex('1+\\left(2+3\\right]+4');
        mq.keystroke('Left Left Left Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('rendering paren group 1+(2+3)+4 from LaTeX then backspacing close-paren then open-paren', function() {
        mq.latex('1+\\left(2+3\\right)+4');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Left Left Backspace');
        assertLatex('1+\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('rendering paren group 1+(2+3)+4 from LaTeX then backspacing open-paren', function() {
        mq.latex('1+\\left(2+3\\right)+4');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Left Left Left Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('wrapping selection in parens 1+(2+3)+4 then backspacing close-paren then open-paren', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Left Left Shift-Left Shift-Left Shift-Left').typedText(')');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Backspace');
        assertLatex('1+\\left(2+3+4\\right)');
        mq.keystroke('Left Left Left Backspace');
        assertLatex('1+2+3+4');
      });

      test('wrapping selection in parens 1+(2+3)+4 then backspacing open-paren', function() {
        mq.typedText('1+2+3+4');
        assertLatex('1+2+3+4');
        mq.keystroke('Left Left Shift-Left Shift-Left Shift-Left').typedText('(');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Backspace');
        assertLatex('1+2+3+4');
      });

      test('backspacing close-bracket of 1+(2+3] (nothing after) then typing', function() {
        mq.typedText('1+(2+3]');
        assertLatex('1+\\left(2+3\\right]');
        mq.keystroke('Backspace');
        assertLatex('1+\\left(2+3\\right)');
        mq.typedText('+4');
        assertLatex('1+\\left(2+3+4\\right)');
      });

      test('backspacing open-paren of (2+3]+4 (nothing before) then typing', function() {
        mq.typedText('(2+3]+4');
        assertLatex('\\left(2+3\\right]+4');
        mq.keystroke('Home Right Backspace');
        assertLatex('2+3+4');
        mq.typedText('1+');
        assertLatex('1+2+3+4');
      });

      test('backspacing paren containing a one-sided paren 0+[(1+2)+3]+4', function() {
        mq.typedText('0+[1+2+3]+4');
        assertLatex('0+\\left[1+2+3\\right]+4');
        mq.keystroke('Left Left Left Left Left').typedText(')');
        assertLatex('0+\\left[\\left(1+2\\right)+3\\right]+4');
        mq.keystroke('Right Right Right Backspace');
        assertLatex('0+\\left[1+2\\right)+3+4');
      });

      test('backspacing paren inside a one-sided paren (0+[1+2]+3)+4', function() {
        mq.typedText('0+[1+2]+3)+4');
        assertLatex('\\left(0+\\left[1+2\\right]+3\\right)+4');
        mq.keystroke('Left Left Left Left Left Backspace');
        assertLatex('0+\\left[1+2+3\\right)+4');
      });

      test('backspacing paren containing and inside a one-sided paren (([1+2]))', function() {
        mq.typedText('(1+2))');
        assertLatex('\\left(\\left(1+2\\right)\\right)');
        mq.keystroke('Left Left').typedText(']');
        assertLatex('\\left(\\left(\\left[1+2\\right]\\right)\\right)');
        mq.keystroke('Right Backspace');
        assertLatex('\\left(\\left(1+2\\right]\\right)');
        mq.keystroke('Backspace');
        assertLatex('\\left(1+2\\right)');
      });

      test('auto-expanding calls .siblingCreated() on new siblings 1+((2+3))', function() {
        mq.typedText('1+((2+3))');
        assertLatex('1+\\left(\\left(2+3\\right)\\right)');
        mq.keystroke('Left Left Left Left Left Left Del');
        assertLatex('1+\\left(\\left(2+3\\right)\\right)');
        mq.keystroke('Left Left Del');
        assertLatex('\\left(1+\\left(2+3\\right)\\right)');
        // now check that the inner open-paren isn't still a ghost
        mq.keystroke('Right Right Right Right Del');
        assertLatex('1+\\left(2+3\\right)');
      });

      test('that unwrapping calls .siblingCreated() on new siblings ((1+2)+(3+4))+5', function() {
        mq.typedText('(1+2+3+4)+5');
        assertLatex('\\left(1+2+3+4\\right)+5');
        mq.keystroke('Home Right Right Right Right').typedText(')');
        assertLatex('\\left(\\left(1+2\\right)+3+4\\right)+5');
        mq.keystroke('Right').typedText('(');
        assertLatex('\\left(\\left(1+2\\right)+\\left(3+4\\right)\\right)+5');
        mq.keystroke('Right Right Right Right Right Backspace');
        assertLatex('\\left(1+2\\right)+\\left(3+4\\right)+5');
        mq.keystroke('Left Left Left Left Backspace');
        assertLatex('\\left(1+2\\right)+3+4+5');
      });

      suite('pipes', function() {
        test('typing then backspacing a pipe in the middle of 1+2+3+4', function() {
          mq.typedText('1+2+3+4');
          assertLatex('1+2+3+4');
          mq.keystroke('Left Left Left').typedText('|');
          assertLatex('1+2+\\left|3+4\\right|');
          mq.keystroke('Backspace');
          assertLatex('1+2+3+4');
        });

        test('backspacing close-pipe then open-pipe of 1+|2+3|+4', function() {
          mq.typedText('1+|2+3|+4');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('1+\\left|2+3+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('backspacing open-pipe of 1+|2+3|+4', function() {
          mq.typedText('1+|2+3|+4');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('backspacing close-pipe then open-pipe of 1+|2+3| (nothing after pipe pair)', function() {
          mq.typedText('1+|2+3|');
          assertLatex('1+\\left|2+3\\right|');
          mq.keystroke('Backspace');
          assertLatex('1+\\left|2+3\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3');
        });

        test('backspacing open-pipe of 1+|2+3| (nothing after pipe pair)', function() {
          mq.typedText('1+|2+3|');
          assertLatex('1+\\left|2+3\\right|');
          mq.keystroke('Left Left Left Left Backspace');
          assertLatex('1+2+3');
        });

        test('backspacing close-pipe then open-pipe of |2+3|+4 (nothing before pipe pair)', function() {
          mq.typedText('|2+3|+4');
          assertLatex('\\left|2+3\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('\\left|2+3+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('2+3+4');
        });

        test('backspacing open-pipe of |2+3|+4 (nothing before pipe pair)', function() {
          mq.typedText('|2+3|+4');
          assertLatex('\\left|2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left Backspace');
          assertLatex('2+3+4');
        });

        function assertParenBlockNonEmpty() {
          var parenBlock = $(mq.el()).find('.mq-paren+span');
          assert.equal(parenBlock.length, 1, 'exactly 1 paren block');
          assert.ok(!parenBlock.hasClass('mq-empty'),
                    'paren block auto-expanded, should no longer be gray');
        }

        test('backspacing close-pipe then open-pipe of 1+||+4 (empty pipe pair)', function() {
          mq.typedText('1+||+4');
          assertLatex('1+\\left|\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('1+\\left|+4\\right|');
          assertParenBlockNonEmpty();
          mq.keystroke('Backspace');
          assertLatex('1++4');
        });

        test('backspacing open-pipe of 1+||+4 (empty pipe pair)', function() {
          mq.typedText('1+||+4');
          assertLatex('1+\\left|\\right|+4');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1++4');
        });

        test('backspacing close-pipe then open-pipe of 1+|| (empty pipe pair, nothing after)', function() {
          mq.typedText('1+||');
          assertLatex('1+\\left|\\right|');
          mq.keystroke('Backspace');
          assertLatex('1+\\left|\\right|');
          mq.keystroke('Backspace');
          assertLatex('1+');
        });

        test('backspacing open-pipe of 1+|| (empty pipe pair, nothing after)', function() {
          mq.typedText('1+||');
          assertLatex('1+\\left|\\right|');
          mq.keystroke('Left Backspace');
          assertLatex('1+');
        });

        test('backspacing close-pipe then open-pipe of ||+4 (empty pipe pair, nothing before)', function() {
          mq.typedText('||+4');
          assertLatex('\\left|\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('\\left|+4\\right|');
          assertParenBlockNonEmpty();
          mq.keystroke('Backspace');
          assertLatex('+4');
        });

        test('backspacing open-pipe of ||+4 (empty pipe pair, nothing before)', function() {
          mq.typedText('||+4');
          assertLatex('\\left|\\right|+4');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('+4');
        });

        test('rendering pipe pair 1+|2+3|+4 from LaTeX then backspacing close-pipe then open-pipe', function() {
          mq.latex('1+\\left|2+3\\right|+4');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('1+\\left|2+3+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('rendering pipe pair 1+|2+3|+4 from LaTeX then backspacing open-pipe', function() {
          mq.latex('1+\\left|2+3\\right|+4');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('rendering mismatched paren/pipe group 1+|2+3)+4 from LaTeX then backspacing close-paren then open-pipe', function() {
          mq.latex('1+\\left|2+3\\right)+4');
          assertLatex('1+\\left|2+3\\right)+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('1+\\left|2+3+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('rendering mismatched paren/pipe group 1+|2+3)+4 from LaTeX then backspacing open-pipe', function() {
          mq.latex('1+\\left|2+3\\right)+4');
          assertLatex('1+\\left|2+3\\right)+4');
          mq.keystroke('Left Left Left Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('rendering mismatched paren/pipe group 1+(2+3|+4 from LaTeX then backspacing close-pipe then open-paren', function() {
          mq.latex('1+\\left(2+3\\right|+4');
          assertLatex('1+\\left(2+3\\right|+4');
          mq.keystroke('Left Left Backspace');
          assertLatex('1+\\left(2+3+4\\right)');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('rendering mismatched paren/pipe group 1+(2+3|+4 from LaTeX then backspacing open-paren', function() {
          mq.latex('1+\\left(2+3\\right|+4');
          assertLatex('1+\\left(2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('wrapping selection in pipes 1+|2+3|+4 then backspacing open-pipe', function() {
          mq.typedText('1+2+3+4');
          assertLatex('1+2+3+4');
          mq.keystroke('Left Left Shift-Left Shift-Left Shift-Left').typedText('|');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Backspace');
          assertLatex('1+2+3+4');
        });

        test('wrapping selection in pipes 1+|2+3|+4 then backspacing close-pipe then open-pipe', function() {
          mq.typedText('1+2+3+4');
          assertLatex('1+2+3+4');
          mq.keystroke('Left Left Shift-Left Shift-Left Shift-Left').typedText('|');
          assertLatex('1+\\left|2+3\\right|+4');
          mq.keystroke('Tab Backspace');
          assertLatex('1+\\left|2+3+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('1+2+3+4');
        });

        test('backspacing close-pipe of 1+|2+3| (nothing after) then typing', function() {
          mq.typedText('1+|2+3|');
          assertLatex('1+\\left|2+3\\right|');
          mq.keystroke('Backspace');
          assertLatex('1+\\left|2+3\\right|');
          mq.typedText('+4');
          assertLatex('1+\\left|2+3+4\\right|');
        });

        test('backspacing open-pipe of |2+3|+4 (nothing before) then typing', function() {
          mq.typedText('|2+3|+4');
          assertLatex('\\left|2+3\\right|+4');
          mq.keystroke('Home Right Backspace');
          assertLatex('2+3+4');
          mq.typedText('1+');
          assertLatex('1+2+3+4');
        });

        test('backspacing pipe containing a one-sided pipe 0+|1+|2+3||+4', function() {
          mq.typedText('0+|1+2+3|+4');
          assertLatex('0+\\left|1+2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left').typedText('|');
          assertLatex('0+\\left|1+\\left|2+3\\right|\\right|+4');
          mq.keystroke('Shift-Tab Shift-Tab Del');
          assertLatex('0+1+\\left|2+3\\right|+4');
        });

        test('backspacing pipe inside a one-sided pipe 0+|1+|2+3|+4|', function() {
          mq.typedText('0+1+|2+3|+4');
          assertLatex('0+1+\\left|2+3\\right|+4');
          mq.keystroke('Home Right Right').typedText('|');
          assertLatex('0+\\left|1+\\left|2+3\\right|+4\\right|');
          mq.keystroke('Right Right Del');
          assertLatex('0+\\left|1+2+3\\right|+4');
        });

        test('backspacing pipe containing and inside a one-sided pipe |0+|1+|2+3||+4|', function() {
          mq.typedText('0+|1+2+3|+4');
          assertLatex('0+\\left|1+2+3\\right|+4');
          mq.keystroke('Home').typedText('|');
          assertLatex('\\left|0+\\left|1+2+3\\right|+4\\right|');
          mq.keystroke('Right Right Right Right Right').typedText('|');
          assertLatex('\\left|0+\\left|1+\\left|2+3\\right|\\right|+4\\right|');
          mq.keystroke('Left Left Left Backspace');
          assertLatex('\\left|0+1+\\left|2+3\\right|+4\\right|');
        });

        test('backspacing pipe containing a one-sided pipe facing same way 0+||1+2||+3', function() {
          mq.typedText('0+|1+2|+3');
          assertLatex('0+\\left|1+2\\right|+3');
          mq.keystroke('Home Right Right Right').typedText('|');
          assertLatex('0+\\left|\\left|1+2\\right|\\right|+3');
          mq.keystroke('Tab Tab Backspace');
          assertLatex('0+\\left|\\left|1+2\\right|+3\\right|');
        });

        test('backspacing pipe inside a one-sided pipe facing same way 0+|1+|2+3|+4|', function() {
          mq.typedText('0+1+|2+3|+4');
          assertLatex('0+1+\\left|2+3\\right|+4');
          mq.keystroke('Home Right Right').typedText('|');
          assertLatex('0+\\left|1+\\left|2+3\\right|+4\\right|');
          mq.keystroke('Right Right Right Right Right Right Right Backspace');
          assertLatex('0+\\left|1+\\left|2+3+4\\right|\\right|');
        });

        test('backspacing open-paren of mismatched paren/pipe group containing a one-sided pipe 0+(1+|2+3||+4', function() {
          mq.latex('0+\\left(1+2+3\\right|+4');
          assertLatex('0+\\left(1+2+3\\right|+4');
          mq.keystroke('Left Left Left Left Left Left').typedText('|');
          assertLatex('0+\\left(1+\\left|2+3\\right|\\right|+4');
          mq.keystroke('Shift-Tab Shift-Tab Del');
          assertLatex('0+1+\\left|2+3\\right|+4');
        });

        test('backspacing open-paren of mismatched paren/pipe group inside a one-sided pipe 0+|1+(2+3|+4|', function() {
          mq.latex('0+1+\\left(2+3\\right|+4');
          assertLatex('0+1+\\left(2+3\\right|+4');
          mq.keystroke('Home Right Right').typedText('|');
          assertLatex('0+\\left|1+\\left(2+3\\right|+4\\right|');
          mq.keystroke('Right Right Del');
          assertLatex('0+\\left|1+2+3\\right|+4');
        });
      });
    }

    suite('typing outside ghost paren', function() {
      test('typing outside ghost paren solidifies ghost 1+(2+3)', function() {
        mq.typedText('1+(2+3');
        assertLatex('1+\\left(2+3\\right)');
        mq.keystroke('Right').typedText('+4');
        assertLatex('1+\\left(2+3\\right)+4');
        mq.keystroke('Left Left Left Left Left Left Left Del');
        assertLatex('\\left(1+2+3\\right)+4');
      });

      test('selected and replaced by LiveFraction solidifies ghosts (1+2)/( )', function() {
        mq.typedText('1+2)/');
        assertLatex('\\frac{\\left(1+2\\right)}{ }');
        mq.keystroke('Left Backspace');
        assertLatex('\\frac{\\left(1+2\\right)}{ }');
      });

      test('close paren group by typing close-bracket outside ghost paren (1+2]', function() {
        mq.typedText('(1+2');
        assertLatex('\\left(1+2\\right)');
        mq.keystroke('Right').typedText(']');
        assertLatex('\\left(1+2\\right]');
      });

      test('close adjacent paren group before containing paren group (1+(2+3])', function() {
        mq.typedText('(1+(2+3');
        assertLatex('\\left(1+\\left(2+3\\right)\\right)');
        mq.keystroke('Right').typedText(']');
        assertLatex('\\left(1+\\left(2+3\\right]\\right)');
        mq.typedText(']');
        assertLatex('\\left(1+\\left(2+3\\right]\\right]');
      });

      test('can type close-bracket on solid side of one-sided paren [](1+2)', function() {
        mq.typedText('(1+2');
        assertLatex('\\left(1+2\\right)');
        mq.moveToLeftEnd().typedText(']');
        assertLatex('\\left[\\right]\\left(1+2\\right)');
      });

      suite('pipes', function() {
        test('close pipe pair from outside to the right |1+2|', function() {
          mq.typedText('|1+2');
          assertLatex('\\left|1+2\\right|');
          mq.keystroke('Right').typedText('|');
          assertLatex('\\left|1+2\\right|');
          mq.keystroke('Home Del');
          assertLatex('\\left|1+2\\right|');
        });

        test('close pipe pair from outside to the left |1+2|', function() {
          mq.typedText('|1+2|');
          assertLatex('\\left|1+2\\right|');
          mq.keystroke('Home Del');
          assertLatex('\\left|1+2\\right|');
          mq.keystroke('Left').typedText('|');
          assertLatex('\\left|1+2\\right|');
          mq.keystroke('Ctrl-End Backspace');
          assertLatex('\\left|1+2\\right|');
        });

        test('can type pipe on solid side of one-sided pipe ||||', function() {
          mq.typedText('|');
          assertLatex('\\left|\\right|');
          mq.moveToLeftEnd().typedText('|');
          assertLatex('\\left|\\left|\\right|\\right|');
        });
      });
    });
  });

  suite('autoCommands', function() {
    setup(function() {
      MQ.config({
        autoCommands: 'pi tau phi theta Gamma sum prod sqrt nthroot'
      });
    });

    test('individual commands', function(){
      mq.typedText('sum' + 'n=0');
      mq.keystroke('Up').typedText('100').keystroke('Right');
      assertLatex('\\sum_{n=0}^{100}');
      mq.keystroke('Ctrl-Backspace');

      mq.typedText('prod');
      mq.typedText('n=0').keystroke('Up').typedText('100').keystroke('Right');
      assertLatex('\\prod_{n=0}^{100}');
      mq.keystroke('Ctrl-Backspace');

      mq.typedText('sqrt');
      mq.typedText('100').keystroke('Right');
      assertLatex('\\sqrt{100}');
      mq.keystroke('Ctrl-Backspace');

      mq.typedText('nthroot');
      mq.typedText('n').keystroke('Right').typedText('100').keystroke('Right');
      assertLatex('\\sqrt[n]{100}');
      mq.keystroke('Ctrl-Backspace');

      mq.typedText('pi');
      assertLatex('\\pi');
      mq.keystroke('Backspace');

      mq.typedText('tau');
      assertLatex('\\tau');
      mq.keystroke('Backspace');

      mq.typedText('phi');
      assertLatex('\\phi');
      mq.keystroke('Backspace');

      mq.typedText('theta');
      assertLatex('\\theta');
      mq.keystroke('Backspace');

      mq.typedText('Gamma');
      assertLatex('\\Gamma');
      mq.keystroke('Backspace');
    });

    test('sequences of auto-commands and other assorted characters', function() {
      mq.typedText('sin' + 'pi');
      assertLatex('\\sin\\pi');
      mq.keystroke('Left Backspace');
      assertLatex('si\\pi');
      mq.keystroke('Left').typedText('p');
      assertLatex('spi\\pi');
      mq.typedText('i');
      assertLatex('s\\pi i\\pi');
      mq.typedText('p');
      assertLatex('s\\pi pi\\pi');
      mq.keystroke('Right').typedText('n');
      assertLatex('s\\pi pin\\pi');
      mq.keystroke('Left Left Left').typedText('s');
      assertLatex('s\\pi spin\\pi');
      mq.keystroke('Backspace');
      assertLatex('s\\pi pin\\pi');
      mq.keystroke('Del').keystroke('Backspace');
      assertLatex('\\sin\\pi');
    });

    test('command contains non-letters', function() {
      assert.throws(function() { MQ.config({ autoCommands: 'e1' }); });
    });

    test('command length less than 2', function() {
      assert.throws(function() { MQ.config({ autoCommands: 'e' }); });
    });

    test('command is a built-in operator name', function() {
      var cmds = ('Pr arg deg det dim exp gcd hom inf ker lg lim ln log max min sup'
                  + ' limsup liminf injlim projlim Pr').split(' ');
      for (var i = 0; i < cmds.length; i += 1) {
        assert.throws(function() { MQ.config({ autoCommands: cmds[i] }) },
                      'MQ.config({ autoCommands: "'+cmds[i]+'" })');
      }
    });

    test('built-in operator names even after auto-operator names overridden', function() {
      MQ.config({ autoOperatorNames: 'sin inf arcosh cosh cos cosec csc' });
        // ^ happen to be the ones required by autoOperatorNames.test.js
      var cmds = 'Pr arg deg det exp gcd inf lg lim ln log max min sup'.split(' ');
      for (var i = 0; i < cmds.length; i += 1) {
        assert.throws(function() { MQ.config({ autoCommands: cmds[i] }) },
                      'MQ.config({ autoCommands: "'+cmds[i]+'" })');
      }
    });

    suite('command list not perfectly space-delimited', function() {
      test('double space', function() {
        assert.throws(function() { MQ.config({ autoCommands: 'pi  theta' }); });
      });

      test('leading space', function() {
        assert.throws(function() { MQ.config({ autoCommands: ' pi' }); });
      });

      test('trailing space', function() {
        assert.throws(function() { MQ.config({ autoCommands: 'pi ' }); });
      });
    });
  });

  suite('inequalities', function() {
    // assertFullyFunctioningInequality() checks not only that the inequality
    // has the right LaTeX and when you backspace it has the right LaTeX,
    // but also that when you backspace you get the right state such that
    // you can either type = again to get the non-strict inequality again,
    // or backspace again and it'll delete correctly.
    function assertFullyFunctioningInequality(nonStrict, strict) {
      assertLatex(nonStrict);
      mq.keystroke('Backspace');
      assertLatex(strict);
      mq.typedText('=');
      assertLatex(nonStrict);
      mq.keystroke('Backspace');
      assertLatex(strict);
      mq.keystroke('Backspace');
      assertLatex('');
    }
    test('typing and backspacing <= and >=', function() {
      mq.typedText('<');
      assertLatex('<');
      mq.typedText('=');
      assertFullyFunctioningInequality('\\le', '<');

      mq.typedText('>');
      assertLatex('>');
      mq.typedText('=');
      assertFullyFunctioningInequality('\\ge', '>');

      mq.typedText('<<>>==>><<==');
      assertLatex('<<>\\ge=>><\\le=');
    });

    test('typing ≤ and ≥ chars directly', function() {
      mq.typedText('≤');
      assertFullyFunctioningInequality('\\le', '<');

      mq.typedText('≥');
      assertFullyFunctioningInequality('\\ge', '>');
    });

    suite('rendered from LaTeX', function() {
      test('control sequences', function() {
        mq.latex('\\le');
        assertFullyFunctioningInequality('\\le', '<');

        mq.latex('\\ge');
        assertFullyFunctioningInequality('\\ge', '>');
      });

      test('≤ and ≥ chars', function() {
        mq.latex('≤');
        assertFullyFunctioningInequality('\\le', '<');

        mq.latex('≥');
        assertFullyFunctioningInequality('\\ge', '>');
      });
    });
  });

  suite('SupSub behavior options', function() {
    test('charsThatBreakOutOfSupSub', function() {
      assert.equal(mq.typedText('x^2n+y').latex(), 'x^{2n+y}');
      mq.latex('');
      assert.equal(mq.typedText('x^+2n').latex(), 'x^{+2n}');
      mq.latex('');
      assert.equal(mq.typedText('x^-2n').latex(), 'x^{-2n}');
      mq.latex('');
      assert.equal(mq.typedText('x^=2n').latex(), 'x^{=2n}');
      mq.latex('');

      MQ.config({ charsThatBreakOutOfSupSub: '+-=<>' });

      assert.equal(mq.typedText('x^2n+y').latex(), 'x^{2n}+y');
      mq.latex('');

      // Unary operators never break out of exponents.
      assert.equal(mq.typedText('x^+2n').latex(), 'x^{+2n}');
      mq.latex('');
      assert.equal(mq.typedText('x^-2n').latex(), 'x^{-2n}');
      mq.latex('');
      assert.equal(mq.typedText('x^=2n').latex(), 'x^{=2n}');
      mq.latex('');

      // Only break out of exponents if cursor at the end, don't
      // jump from the middle of the exponent out to the right.
      assert.equal(mq.typedText('x^ab').latex(), 'x^{ab}');
      assert.equal(mq.keystroke('Left').typedText('+').latex(), 'x^{a+b}');
      mq.latex('');
    });
    test('supSubsRequireOperand', function() {
      assert.equal(mq.typedText('^').latex(), '^{ }');
      assert.equal(mq.typedText('2').latex(), '^2');
      assert.equal(mq.typedText('n').latex(), '^{2n}');
      mq.latex('');
      assert.equal(mq.typedText('x').latex(), 'x');
      assert.equal(mq.typedText('^').latex(), 'x^{ }');
      assert.equal(mq.typedText('2').latex(), 'x^2');
      assert.equal(mq.typedText('n').latex(), 'x^{2n}');
      mq.latex('');
      assert.equal(mq.typedText('x').latex(), 'x');
      assert.equal(mq.typedText('^').latex(), 'x^{ }');
      assert.equal(mq.typedText('^').latex(), 'x^{^{ }}');
      assert.equal(mq.typedText('2').latex(), 'x^{^2}');
      assert.equal(mq.typedText('n').latex(), 'x^{^{2n}}');

      mq.latex('');
      MQ.config({ supSubsRequireOperand: true });

      assert.equal(mq.typedText('^').latex(), '');
      assert.equal(mq.typedText('2').latex(), '2');
      assert.equal(mq.typedText('n').latex(), '2n');
      mq.latex('');
      assert.equal(mq.typedText('x').latex(), 'x');
      assert.equal(mq.typedText('^').latex(), 'x^{ }');
      assert.equal(mq.typedText('2').latex(), 'x^2');
      assert.equal(mq.typedText('n').latex(), 'x^{2n}');
      mq.latex('');
      assert.equal(mq.typedText('x').latex(), 'x');
      assert.equal(mq.typedText('^').latex(), 'x^{ }');
      assert.equal(mq.typedText('^').latex(), 'x^{ }');
      assert.equal(mq.typedText('2').latex(), 'x^2');
      assert.equal(mq.typedText('n').latex(), 'x^{2n}');
    });
  });
});
suite('up/down', function() {
  var mq, rootBlock, controller, cursor;
  setup(function() {
    mq = MQ.MathField($('<span></span>').appendTo('#mock')[0]);
    rootBlock = mq.__controller.root;
    controller = mq.__controller;
    cursor = controller.cursor;
  });
  teardown(function() {
    $(mq.el()).remove();
  });

  test('up/down in out of exponent', function() {
    controller.renderLatexMath('x^{nm}');
    var exp = rootBlock.ends[R],
      expBlock = exp.ends[L];
    assert.equal(exp.latex(), '^{nm}', 'right end el is exponent');
    assert.equal(cursor.parent, rootBlock, 'cursor is in root block');
    assert.equal(cursor[L], exp, 'cursor is at the end of root block');

    mq.keystroke('Up');
    assert.equal(cursor.parent, expBlock, 'cursor up goes into exponent');

    mq.keystroke('Down');
    assert.equal(cursor.parent, rootBlock, 'cursor down leaves exponent');
    assert.equal(cursor[L], exp, 'down when cursor at end of exponent puts cursor after exponent');

    mq.keystroke('Up Left Left');
    assert.equal(cursor.parent, expBlock, 'cursor up left stays in exponent');
    assert.equal(cursor[L], 0, 'cursor is at the beginning of exponent');

    mq.keystroke('Down');
    assert.equal(cursor.parent, rootBlock, 'cursor down leaves exponent');
    assert.equal(cursor[R], exp, 'cursor down in beginning of exponent puts cursor before exponent');

    mq.keystroke('Up Right');
    assert.equal(cursor.parent, expBlock, 'cursor up left stays in exponent');
    assert.equal(cursor[L].latex(), 'n', 'cursor is in the middle of exponent');
    assert.equal(cursor[R].latex(), 'm', 'cursor is in the middle of exponent');

    mq.keystroke('Down');
    assert.equal(cursor.parent, rootBlock, 'cursor down leaves exponent');
    assert.equal(cursor[R], exp, 'cursor down in middle of exponent puts cursor before exponent');
  });

  // literally just swapped up and down, exponent with subscript, nm with 12
  test('up/down in out of subscript', function() {
    controller.renderLatexMath('a_{12}');
    var sub = rootBlock.ends[R],
      subBlock = sub.ends[L];
    assert.equal(sub.latex(), '_{12}', 'right end el is subscript');
    assert.equal(cursor.parent, rootBlock, 'cursor is in root block');
    assert.equal(cursor[L], sub, 'cursor is at the end of root block');

    mq.keystroke('Down');
    assert.equal(cursor.parent, subBlock, 'cursor down goes into subscript');

    mq.keystroke('Up');
    assert.equal(cursor.parent, rootBlock, 'cursor up leaves subscript');
    assert.equal(cursor[L], sub, 'up when cursor at end of subscript puts cursor after subscript');

    mq.keystroke('Down Left Left');
    assert.equal(cursor.parent, subBlock, 'cursor down left stays in subscript');
    assert.equal(cursor[L], 0, 'cursor is at the beginning of subscript');

    mq.keystroke('Up');
    assert.equal(cursor.parent, rootBlock, 'cursor up leaves subscript');
    assert.equal(cursor[R], sub, 'cursor up in beginning of subscript puts cursor before subscript');

    mq.keystroke('Down Right');
    assert.equal(cursor.parent, subBlock, 'cursor down left stays in subscript');
    assert.equal(cursor[L].latex(), '1', 'cursor is in the middle of subscript');
    assert.equal(cursor[R].latex(), '2', 'cursor is in the middle of subscript');

    mq.keystroke('Up');
    assert.equal(cursor.parent, rootBlock, 'cursor up leaves subscript');
    assert.equal(cursor[R], sub, 'cursor up in middle of subscript puts cursor before subscript');
  });

  test('up/down into and within fraction', function() {
    controller.renderLatexMath('\\frac{12}{34}');
    var frac = rootBlock.ends[L],
      numer = frac.ends[L],
      denom = frac.ends[R];
    assert.equal(frac.latex(), '\\frac{12}{34}', 'fraction is in root block');
    assert.equal(frac, rootBlock.ends[R], 'fraction is sole child of root block');
    assert.equal(numer.latex(), '12', 'numerator is left end child of fraction');
    assert.equal(denom.latex(), '34', 'denominator is right end child of fraction');

    mq.keystroke('Up');
    assert.equal(cursor.parent, numer, 'cursor up goes into numerator');
    assert.equal(cursor[R], 0, 'cursor up from right of fraction inserts at right end of numerator');

    mq.keystroke('Down');
    assert.equal(cursor.parent, denom, 'cursor down goes into denominator');
    assert.equal(cursor[R], 0, 'cursor down from numerator inserts at right end of denominator');

    mq.keystroke('Up');
    assert.equal(cursor.parent, numer, 'cursor up goes into numerator');
    assert.equal(cursor[R], 0, 'cursor up from denominator inserts at right end of numerator');

    mq.keystroke('Left Left Left');
    assert.equal(cursor.parent, rootBlock, 'cursor outside fraction');
    assert.equal(cursor[R], frac, 'cursor before fraction');

    mq.keystroke('Up');
    assert.equal(cursor.parent, numer, 'cursor up goes into numerator');
    assert.equal(cursor[L], 0, 'cursor up from left of fraction inserts at left end of numerator');

    mq.keystroke('Left');
    assert.equal(cursor.parent, rootBlock, 'cursor outside fraction');
    assert.equal(cursor[R], frac, 'cursor before fraction');

    mq.keystroke('Down');
    assert.equal(cursor.parent, denom, 'cursor down goes into denominator');
    assert.equal(cursor[L], 0, 'cursor down from left of fraction inserts at left end of denominator');
  });

  test('nested subscripts and fractions', function() {
    controller.renderLatexMath('\\frac{d}{dx_{\\frac{24}{36}0}}\\sqrt{x}=x^{\\frac{1}{2}}');
    var exp = rootBlock.ends[R],
      expBlock = exp.ends[L],
      half = expBlock.ends[L],
      halfNumer = half.ends[L],
      halfDenom = half.ends[R];

    mq.keystroke('Left');
    assert.equal(cursor.parent, expBlock, 'cursor left goes into exponent');

    mq.keystroke('Down');
    assert.equal(cursor.parent, halfDenom, 'cursor down goes into denominator of half');

    mq.keystroke('Down');
    assert.equal(cursor.parent, rootBlock, 'down again puts cursor back in root block');
    assert.equal(cursor[L], exp, 'down from end of half puts cursor after exponent');

    var derivative = rootBlock.ends[L],
      dBlock = derivative.ends[L],
      dxBlock = derivative.ends[R],
      sub = dxBlock.ends[R],
      subBlock = sub.ends[L],
      subFrac = subBlock.ends[L],
      subFracNumer = subFrac.ends[L],
      subFracDenom = subFrac.ends[R];

    cursor.insAtLeftEnd(rootBlock);
    mq.keystroke('Down Right Right Down');
    assert.equal(cursor.parent, subBlock, 'cursor in subscript');

    mq.keystroke('Up');
    assert.equal(cursor.parent, subFracNumer, 'cursor up from beginning of subscript goes into subscript fraction numerator');

    mq.keystroke('Up');
    assert.equal(cursor.parent, dxBlock, 'cursor up from subscript fraction numerator goes out of subscript');
    assert.equal(cursor[R], sub, 'cursor up from subscript fraction numerator goes before subscript');

    mq.keystroke('Down Down');
    assert.equal(cursor.parent, subFracDenom, 'cursor in subscript fraction denominator');

    mq.keystroke('Up Up');
    assert.equal(cursor.parent, dxBlock, 'cursor up up from subscript fraction denominator that\s not at right end goes out of subscript');
    assert.equal(cursor[R], sub, 'cursor up up from subscript fraction denominator that\s not at right end goes before subscript');

    cursor.insAtRightEnd(subBlock);
    controller.backspace();
    assert.equal(subFrac[R], 0, 'subscript fraction is at right end');
    assert.equal(cursor[L], subFrac, 'cursor after subscript fraction');

    mq.keystroke('Down');
    assert.equal(cursor.parent, subFracDenom, 'cursor in subscript fraction denominator');

    mq.keystroke('Up Up');
    assert.equal(cursor.parent, dxBlock, 'cursor up up from subscript fraction denominator that is at right end goes out of subscript');
    assert.equal(cursor[L], sub, 'cursor up up from subscript fraction denominator that is at right end goes after subscript');
  });

  test('\\MathQuillMathField{} in a fraction', function() {
    var outer = MQ.MathField(
      $('<span>\\frac{\\MathQuillMathField{n}}{2}</span>').appendTo('#mock')[0]
    );
    var inner = MQ($(outer.el()).find('.mq-editable-field')[0]);

    assert.equal(inner.__controller.cursor.parent, inner.__controller.root);
    inner.keystroke('Down');
    assert.equal(inner.__controller.cursor.parent, inner.__controller.root);

    $(outer.el()).remove();
  });
});
var MQ1 = getInterface(1);
for (var key in MQ1) (function(key, val) {
  if (typeof val === 'function') {
    MathQuill[key] = function() {
      insistOnInterVer();
      return val.apply(this, arguments);
    };
    MathQuill[key].prototype = val.prototype;
  }
  else MathQuill[key] = val;
}(key, MQ1[key]));

}());
