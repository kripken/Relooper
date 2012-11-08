
  var RBUFFER_SIZE = 20*1024*1024;
  var rbuffer = _malloc(RBUFFER_SIZE);
  _rl_set_output_buffer(rbuffer, RBUFFER_SIZE);

  var TBUFFER_SIZE = 10*1024*1024;
  var tbuffer = _malloc(TBUFFER_SIZE);

  var RelooperGlue = {};
  RelooperGlue['init'] = function() {
    this.r = _rl_new_relooper();
  },
  RelooperGlue['addBlock'] = function(text) {
    assert(this.r);
    assert(text.length+1 < TBUFFER_SIZE);
    writeStringToMemory(text, tbuffer);
    var b = _rl_new_block(tbuffer);
    _rl_relooper_add_block(this.r, b);
    return b;
  };
  RelooperGlue['addBranch'] = function(from, to, condition) {
    assert(this.r);
    if (condition) {
      assert(condition.length+1 < TBUFFER_SIZE);
      writeStringToMemory(condition, tbuffer);
      condition = tbuffer;
    }
    _rl_block_add_branch_to(from, to, condition);
  };
  RelooperGlue['render'] = function(entry) {
    assert(this.r);
    assert(entry);
    _rl_relooper_calculate(this.r, entry);
    _rl_relooper_render(this.r);
    var ret = Pointer_stringify(rbuffer);
    _rl_delete_relooper(this.r);
    this.r = 0;
    return ret;
  };
  RelooperGlue['setDebug'] = function(debug) {
    _rl_set_debugging(+!!debug);
  };

  Module['Relooper'] = RelooperGlue;

