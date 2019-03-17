#ifndef __PYWRAPS_CUSTVIEWER__
#define __PYWRAPS_CUSTVIEWER__
//<code(py_kernwin_custview)>
//---------------------------------------------------------------------------
// Base class for all custviewer place_t providers
class custviewer_data_t
{
public:
  virtual void    *get_ud() = 0;
  virtual place_t *get_min() = 0;
  virtual place_t *get_max() = 0;
};

//---------------------------------------------------------------------------
class cvdata_simpleline_t: public custviewer_data_t
{
private:
  strvec_t lines;
  simpleline_place_t pl_min, pl_max;
public:

  void *get_ud()
  {
    return &lines;
  }

  place_t *get_min()
  {
    return &pl_min;
  }

  place_t *get_max()
  {
    return &pl_max;
  }

  strvec_t &get_lines()
  {
    return lines;
  }

  void set_minmax(size_t start=0, size_t end=size_t(-1))
  {
    if ( start == 0 && end == size_t(-1) )
    {
      end = lines.size();
      pl_min.n = 0;
      pl_max.n = end == 0 ? 0 : end - 1;
    }
    else
    {
      pl_min.n = start;
      pl_max.n = end;
    }
  }

  bool set_line(size_t nline, simpleline_t &sl)
  {
    if ( nline >= lines.size() )
      return false;
    lines[nline] = sl;
    return true;
  }

  bool del_line(size_t nline)
  {
    if ( nline >= lines.size() )
      return false;
    lines.erase(lines.begin()+nline);
    return true;
  }

  void add_line(simpleline_t &line)
  {
    lines.push_back(line);
  }

  void add_line(const char *str)
  {
    lines.push_back(simpleline_t(str));
  }

  bool insert_line(size_t nline, simpleline_t &line)
  {
    if ( nline >= lines.size() )
      return false;
    lines.insert(lines.begin()+nline, line);
    return true;
  }

  bool patch_line(size_t nline, size_t offs, int value)
  {
    if ( nline >= lines.size() )
      return false;
    qstring &L = lines[nline].line;
    L[offs] = (uchar) value & 0xFF;
    return true;
  }

  const size_t to_lineno(place_t *pl) const
  {
    return ((simpleline_place_t *)pl)->n;
  }

  bool curline(place_t *pl, size_t *n)
  {
    if ( pl == NULL )
      return false;

    *n = to_lineno(pl);
    return true;
  }

  simpleline_t *get_line(size_t nline)
  {
    return nline >= lines.size() ? NULL : &lines[nline];
  }

  simpleline_t *get_line(place_t *pl)
  {
    return pl == NULL ? NULL : get_line(((simpleline_place_t *)pl)->n);
  }

  const size_t count() const
  {
    return lines.size();
  }

  void clear_lines()
  {
    lines.clear();
    set_minmax();
  }
};

//---------------------------------------------------------------------------
// FIXME: This should inherit py_view_base.hpp's py_customidamemo_t,
// just like py_graph.hpp's py_graph_t does.
// There should be a way to "merge" the two mechanisms; they are similar.
class customviewer_t
{
protected:
  qstring _title;
  TWidget *_cv;
  custviewer_data_t *_data;
  int _features;
  custom_viewer_handlers_t handlers;

  enum
  {
    HAVE_HINT     = 0x0001,
    HAVE_KEYDOWN  = 0x0002,
    HAVE_DBLCLICK = 0x0004,
    HAVE_CURPOS   = 0x0008,
    HAVE_CLICK    = 0x0010,
    HAVE_CLOSE    = 0x0020
  };
private:
  struct cvw_popupctx_t
  {
    size_t menu_id;
    customviewer_t *cv;
    cvw_popupctx_t(): menu_id(0), cv(NULL) {}
    cvw_popupctx_t(size_t mid, customviewer_t *v): menu_id(mid), cv(v) {}
  };
  typedef std::map<unsigned int, cvw_popupctx_t> cvw_popupmap_t;
  static size_t _global_popup_id;
  qstring _curline;

  static bool idaapi s_cv_keydown(
          TWidget * /*cv*/,
          int vk_key,
          int shift,
          void *ud)
  {
    PYW_GIL_GET;
    customviewer_t *_this = (customviewer_t *)ud;
    return _this->on_keydown(vk_key, shift);
  }

  // The user clicked
  static bool idaapi s_cv_click(TWidget * /*cv*/, int shift, void *ud)
  {
    PYW_GIL_GET;
    customviewer_t *_this = (customviewer_t *)ud;
    return _this->on_click(shift);
  }

  // The user double clicked
  static bool idaapi s_cv_dblclick(TWidget * /*cv*/, int shift, void *ud)
  {
    PYW_GIL_GET;
    customviewer_t *_this = (customviewer_t *)ud;
    return _this->on_dblclick(shift);
  }

  // Cursor position has been changed
  static void idaapi s_cv_curpos(TWidget * /*cv*/, void *ud)
  {
    PYW_GIL_GET;
    customviewer_t *_this = (customviewer_t *)ud;
    _this->on_curpos_changed();
  }

  //--------------------------------------------------------------------------
  static ssize_t idaapi s_ui_cb(void *ud, int code, va_list va)
  {
    // This hook gets called from the kernel. Ensure we hold the GIL.
    PYW_GIL_GET;
    customviewer_t *_this = (customviewer_t *)ud;
    switch ( code )
    {
      case ui_get_custom_viewer_hint:
        {
          qstring &hint = *va_arg(va, qstring *);
          TWidget *viewer = va_arg(va, TWidget *);
          place_t *place = va_arg(va, place_t *);
          int *important_lines = va_arg(va, int *);
          if ( (_this->_features & HAVE_HINT) == 0
            || place == NULL
            || _this->_cv != viewer )
          {
            return 0;
          }
          return _this->on_hint(place, important_lines, hint) ? 1 : 0;
        }

      case ui_widget_invisible:
        {
          TWidget *widget = va_arg(va, TWidget *);
          if ( _this->_cv != widget )
            break;
        }
        // fallthrough...
      case ui_term:
        idapython_unhook_from_notification_point(HT_UI, s_ui_cb, _this);
        _this->on_close();
        _this->on_post_close();
        break;
    }

    return 0;
  }

  void on_post_close()
  {
    init_vars();
  }

public:

  inline TWidget *get_widget() { return _cv; }

  //
  // All the overridable callbacks
  //

  // OnClick
  virtual bool on_click(int /*shift*/) { return false; }

  // OnDblClick
  virtual bool on_dblclick(int /*shift*/) { return false; }

  // OnCurorPositionChanged
  virtual void on_curpos_changed() {}

  // OnHostFormClose
  virtual void on_close() {}

  // OnKeyDown
  virtual bool on_keydown(int /*vk_key*/, int /*shift*/) { return false; }

  // OnHint
  virtual bool on_hint(place_t * /*place*/, int * /*important_lines*/, qstring &/*hint*/) { return false; }

  // OnPopupMenuClick
  virtual bool on_popup_menu(size_t menu_id) { return false; }

  void init_vars()
  {
    _data = NULL;
    _features = 0;
    _curline.clear();
    _cv = NULL;
  }

  customviewer_t()
  {
    init_vars();
  }

  ~customviewer_t()
  {
  }

  //--------------------------------------------------------------------------
  void close()
  {
    if ( _cv != NULL )
      close_widget(_cv, WCLS_SAVE | WCLS_CLOSE_LATER);
  }

  //--------------------------------------------------------------------------
  bool set_range(
    const place_t *minplace = NULL,
    const place_t *maxplace = NULL)
  {
    if ( _cv == NULL )
      return false;

    set_custom_viewer_range(
      _cv,
      minplace == NULL ? _data->get_min() : minplace,
      maxplace == NULL ? _data->get_max() : maxplace);
    return true;
  }

  place_t *get_place(
    bool mouse = false,
    int *x = 0,
    int *y = 0)
  {
    return _cv == NULL ? NULL : get_custom_viewer_place(_cv, mouse, x, y);
  }

  //--------------------------------------------------------------------------
  bool refresh()
  {
    if ( _cv == NULL )
      return false;

    refresh_custom_viewer(_cv);
    return true;
  }

  //--------------------------------------------------------------------------
  bool refresh_current()
  {
    return refresh();
  }

  //--------------------------------------------------------------------------
  bool get_current_word(bool mouse, qstring &word)
  {
    // query the cursor position
    int x, y;
    if ( get_place(mouse, &x, &y) == NULL )
      return false;

    // query the line at the cursor
    const char *line = get_current_line(mouse, true);
    if ( line == NULL )
      return false;

    if ( x >= (int)strlen(line) )
      return false;

    // find the beginning of the word
    const char *ptr = line + x;
    while ( ptr > line && !qisspace(ptr[-1]) )
      ptr--;

    // find the end of the word
    const char *begin = ptr;
    ptr = line + x;
    while ( !qisspace(*ptr) && *ptr != '\0' )
      ptr++;

    word.qclear();
    word.append(begin, ptr-begin);
    return true;
  }

  //--------------------------------------------------------------------------
  const char *get_current_line(bool mouse, bool notags)
  {
    const char *r = get_custom_viewer_curline(_cv, mouse);
    if ( r == NULL || !notags )
      return r;

    _curline = r;
    tag_remove(&_curline);
    return _curline.c_str();
  }

  //--------------------------------------------------------------------------
  bool is_focused()
  {
    return get_current_viewer() == _cv;
  }

  //--------------------------------------------------------------------------
  bool jumpto(place_t *place, int x, int y)
  {
    return ::jumpto(_cv, place, x, y);
  }

  bool create(const char *title, int features, custviewer_data_t *data)
  {
    // Already created? (in the instance)
    if ( _cv != NULL )
      return true;

    // Already created? (in IDA windows list)
    TWidget *found = find_widget(title);
    if ( found != NULL )
      return false;

    _title    = title;
    _data     = data;
    _features = features;

    //
    // Prepare handlers
    //
    if ( (features & HAVE_KEYDOWN) != 0 )
      handlers.keyboard = s_cv_keydown;

    if ( (features & HAVE_CLICK) != 0 )
      handlers.click = s_cv_click;

    if ( (features & HAVE_DBLCLICK) != 0 )
      handlers.dblclick = s_cv_dblclick;

    if ( (features & HAVE_CURPOS) != 0 )
      handlers.curpos = s_cv_curpos;

    // Create the viewer
    _cv = create_custom_viewer(
      title,
      _data->get_min(),
      _data->get_max(),
      _data->get_min(),
      (const renderer_info_t *) NULL,
      _data->get_ud(),
      &handlers,
      this);

    // Hook to UI notifications (for TWidget close event)
    idapython_hook_to_notification_point(HT_UI, s_ui_cb, this);

    return true;
  }

  //--------------------------------------------------------------------------
  bool show()
  {
    // Closed already?
    if ( _cv == NULL )
      return false;

    display_widget(_cv, WOPN_TAB|WOPN_RESTORE);
    return true;
  }
};

size_t customviewer_t::_global_popup_id = 0;
//---------------------------------------------------------------------------
class py_simplecustview_t: public customviewer_t
{
private:
  cvdata_simpleline_t data;
  PyObject *py_self, *py_this, *py_last_link;
  int features;

  //-------------------------------------------------------------------------
  static bool get_color(uint32 *out, ref_t obj)
  {
    bool ok = PyLong_Check(obj.o);
    if ( ok )
    {
      *out = uint32(PyLong_AsUnsignedLong(obj.o));
    }
    else
    {
      ok = PyInt_Check(obj.o);
      if ( ok )
        *out = uint32(PyInt_AsLong(obj.o));
    }
    return ok;
  }

  //--------------------------------------------------------------------------
  // Convert a tuple (String, [color, [bgcolor]]) to a simpleline_t
  static bool py_to_simpleline(PyObject *py, simpleline_t &sl)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();

    if ( PyString_Check(py) )
    {
      sl.line = PyString_AsString(py);
      return true;
    }
    Py_ssize_t sz;
    if ( !PyTuple_Check(py) || (sz = PyTuple_Size(py)) <= 0 )
      return false;

    PyObject *py_val = PyTuple_GetItem(py, 0);
    if ( !PyString_Check(py_val) )
      return false;

    sl.line = PyString_AsString(py_val);
    uint32 col;
    if ( sz > 1 && get_color(&col, borref_t(PyTuple_GetItem(py, 1))) )
      sl.color = color_t(col);
    if ( sz > 2 && get_color(&col, borref_t(PyTuple_GetItem(py, 2))) )
      sl.bgcolor = bgcolor_t(col);
    return true;
  }

  //
  // Callbacks
  //
  virtual bool on_click(int shift)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(PyObject_CallMethod(py_self, (char *)S_ON_CLICK, "i", shift));
    PyW_ShowCbErr(S_ON_CLICK);
    return py_result != NULL && PyObject_IsTrue(py_result.o);
  }

  //--------------------------------------------------------------------------
  // OnDblClick
  virtual bool on_dblclick(int shift)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(PyObject_CallMethod(py_self, (char *)S_ON_DBL_CLICK, "i", shift));
    PyW_ShowCbErr(S_ON_DBL_CLICK);
    return py_result != NULL && PyObject_IsTrue(py_result.o);
  }

  //--------------------------------------------------------------------------
  // OnCurorPositionChanged
  virtual void on_curpos_changed()
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(PyObject_CallMethod(py_self, (char *)S_ON_CURSOR_POS_CHANGED, NULL));
    PyW_ShowCbErr(S_ON_CURSOR_POS_CHANGED);
  }

  //--------------------------------------------------------------------------
  // OnHostFormClose
  virtual void on_close()
  {
    if ( py_self != NULL )
    {
      // Call the close method if it is there and the object is still bound
      if ( (features & HAVE_CLOSE) != 0 )
      {
        PYW_GIL_CHECK_LOCKED_SCOPE();
        newref_t py_result(PyObject_CallMethod(py_self, (char *)S_ON_CLOSE, NULL));
        PyW_ShowCbErr(S_ON_CLOSE);
      }

      // Cleanup
      Py_DECREF(py_self);
      py_self = NULL;
    }
  }

  //--------------------------------------------------------------------------
  // OnKeyDown
  virtual bool on_keydown(int vk_key, int shift)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(
            PyObject_CallMethod(
                    py_self,
                    (char *)S_ON_KEYDOWN,
                    "ii",
                    vk_key,
                    shift));

    PyW_ShowCbErr(S_ON_KEYDOWN);
    return py_result != NULL && PyObject_IsTrue(py_result.o);
  }

  //--------------------------------------------------------------------------
  // OnHint
  virtual bool on_hint(place_t *place, int *important_lines, qstring &hint)
  {
    size_t ln = data.to_lineno(place);
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(
            PyObject_CallMethod(
                    py_self,
                    (char *)S_ON_HINT,
                    PY_BV_SZ,
                    bvsz_t(ln)));

    PyW_ShowCbErr(S_ON_HINT);
    bool ok = py_result != NULL && PyTuple_Check(py_result.o) && PyTuple_Size(py_result.o) == 2;
    if ( ok )
    {
      if ( important_lines != NULL )
        *important_lines = PyInt_AsLong(PyTuple_GetItem(py_result.o, 0));
      hint = PyString_AsString(PyTuple_GetItem(py_result.o, 1));
    }
    return ok;
  }

  //--------------------------------------------------------------------------
  // OnPopupMenuClick
  virtual bool on_popup_menu(size_t menu_id)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_result(
            PyObject_CallMethod(
                    py_self,
                    (char *)S_ON_POPUP_MENU,
                    PY_BV_SZ,
                    bvsz_t(menu_id)));
    PyW_ShowCbErr(S_ON_POPUP_MENU);
    return py_result != NULL && PyObject_IsTrue(py_result.o);
  }

  //--------------------------------------------------------------------------
  void refresh_range()
  {
    data.set_minmax();
    set_range();
  }

public:
  py_simplecustview_t()
  {
    py_this = py_self = py_last_link = NULL;
  }
  ~py_simplecustview_t()
  {
  }

  //--------------------------------------------------------------------------
  // Edits an existing line
  bool edit_line(size_t nline, PyObject *py_sl)
  {
    simpleline_t sl;
    if ( !py_to_simpleline(py_sl, sl) )
      return false;

    return data.set_line(nline, sl);
  }

  // Low level: patches a line string directly
  bool patch_line(size_t nline, size_t offs, int value)
  {
    return data.patch_line(nline, offs, value);
  }

  // Insert a line
  bool insert_line(size_t nline, PyObject *py_sl)
  {
    simpleline_t sl;
    if ( !py_to_simpleline(py_sl, sl) )
      return false;
    return data.insert_line(nline, sl);
  }

  // Adds a line tuple
  bool add_line(PyObject *py_sl)
  {
    simpleline_t sl;
    if ( !py_to_simpleline(py_sl, sl) )
      return false;
    data.add_line(sl);
    refresh_range();
    return true;
  }

  //--------------------------------------------------------------------------
  bool del_line(size_t nline)
  {
    bool ok = data.del_line(nline);
    if ( ok )
      refresh_range();
    return ok;
  }

  //--------------------------------------------------------------------------
  // Gets the position and returns a tuple (lineno, x, y)
  PyObject *get_pos(bool mouse)
  {
    place_t *pl;
    int x, y;
    pl = get_place(mouse, &x, &y);
    PYW_GIL_CHECK_LOCKED_SCOPE();
    if ( pl == NULL )
      Py_RETURN_NONE;
    return Py_BuildValue("(" PY_BV_SZ "ii)", bvsz_t(data.to_lineno(pl)), x, y);
  }

  //--------------------------------------------------------------------------
  // Returns the line tuple
  PyObject *get_line(size_t nline)
  {
    simpleline_t *r = data.get_line(nline);
    PYW_GIL_CHECK_LOCKED_SCOPE();
    if ( r == NULL )
      Py_RETURN_NONE;
    return Py_BuildValue("(sII)", r->line.c_str(), (unsigned int)r->color, (unsigned int)r->bgcolor);
  }

  // Returns the count of lines
  const size_t count() const
  {
    return data.count();
  }

  // Clears lines
  void clear()
  {
    data.clear_lines();
    refresh_range();
  }

  //--------------------------------------------------------------------------
  bool jumpto(size_t ln, int x, int y)
  {
    simpleline_place_t l(ln);
    return customviewer_t::jumpto(&l, x, y);
  }

  //--------------------------------------------------------------------------
  // Initializes and links the Python object to this class
  bool init(PyObject *py_link, const char *title)
  {
    // Already created?
    if ( _cv != NULL )
      return true;

    // Probe callbacks
    features = 0;
    static struct
    {
      const char *cb_name;
      int feature;
    } const cbtable[] =
    {
      { S_ON_CLICK,              HAVE_CLICK },
      { S_ON_CLOSE,              HAVE_CLOSE },
      { S_ON_HINT,               HAVE_HINT },
      { S_ON_KEYDOWN,            HAVE_KEYDOWN },
      { S_ON_DBL_CLICK,          HAVE_DBLCLICK },
      { S_ON_CURSOR_POS_CHANGED, HAVE_CURPOS }
    };

    PYW_GIL_CHECK_LOCKED_SCOPE();
    for ( size_t i=0; i < qnumber(cbtable); i++ )
    {
      if ( PyObject_HasAttrString(py_link, cbtable[i].cb_name) )
        features |= cbtable[i].feature;
    }

    if ( !create(title, features, &data) )
      return false;

    // Hold a reference to this object
    py_last_link = py_self = py_link;
    Py_INCREF(py_self);

    // Return a reference to the C++ instance (only once)
    if ( py_this == NULL )
      py_this = PyCObject_FromVoidPtr(this, NULL);

    return true;
  }

  //--------------------------------------------------------------------------
  bool show()
  {
    if ( _cv == NULL && py_last_link != NULL )
    {
      // Re-create the view (with same previous parameters)
      if ( !init(py_last_link, _title.c_str()) )
        return false;
    }
    return customviewer_t::show();
  }

  //--------------------------------------------------------------------------
  bool get_selection(size_t *x1, size_t *y1, size_t *x2, size_t *y2)
  {
    if ( _cv == NULL )
      return false;

    twinpos_t p1, p2;
    if ( !::read_selection(_cv, &p1, &p2) )
      return false;

    if ( y1 != NULL )
      *y1 = data.to_lineno(p1.at);
    if ( y2 != NULL )
      *y2 = data.to_lineno(p2.at);
    if ( x1 != NULL )
      *x1 = size_t(p1.x);
    if ( x2 != NULL )
      *x2 = p2.x;
    return true;
  }

  PyObject *py_get_selection()
  {
    size_t x1, y1, x2, y2;
    PYW_GIL_CHECK_LOCKED_SCOPE();
    if ( !get_selection(&x1, &y1, &x2, &y2) )
      Py_RETURN_NONE;
    return Py_BuildValue(
            "(" PY_BV_SZ PY_BV_SZ PY_BV_SZ PY_BV_SZ ")",
            bvsz_t(x1), bvsz_t(y1), bvsz_t(x2), bvsz_t(y2));
  }

  static py_simplecustview_t *get_this(PyObject *py_this)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    return PyCObject_Check(py_this) ? (py_simplecustview_t *) PyCObject_AsVoidPtr(py_this) : NULL;
  }

  PyObject *get_pythis()
  {
    return py_this;
  }
};

//</code(py_kernwin_custview)>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//<inline(py_kernwin_custview)>
//
// Pywraps Simple Custom Viewer functions
//
PyObject *pyscv_init(PyObject *py_link, const char *title)
{
  PYW_GIL_CHECK_LOCKED_SCOPE();
  py_simplecustview_t *_this = new py_simplecustview_t();
  bool ok = _this->init(py_link, title);
  if ( !ok )
  {
    delete _this;
    Py_RETURN_NONE;
  }
  return _this->get_pythis();
}
#define DECL_THIS py_simplecustview_t *_this = py_simplecustview_t::get_this(py_this)

//--------------------------------------------------------------------------
bool pyscv_refresh(PyObject *py_this)
{
  DECL_THIS;
  if ( _this == NULL )
    return false;
  return _this->refresh();
}

//--------------------------------------------------------------------------
bool pyscv_delete(PyObject *py_this)
{
  DECL_THIS;
  if ( _this == NULL )
    return false;
  _this->close();
  delete _this;
  return true;
}

//--------------------------------------------------------------------------
bool pyscv_refresh_current(PyObject *py_this)
{
  DECL_THIS;
  if ( _this == NULL )
    return false;
  return _this->refresh_current();
}

//--------------------------------------------------------------------------
PyObject *pyscv_get_current_line(PyObject *py_this, bool mouse, bool notags)
{
  DECL_THIS;
  PYW_GIL_CHECK_LOCKED_SCOPE();
  const char *line;
  if ( _this == NULL || (line = _this->get_current_line(mouse, notags)) == NULL )
    Py_RETURN_NONE;
  return PyString_FromString(line);
}

//--------------------------------------------------------------------------
bool pyscv_is_focused(PyObject *py_this)
{
  DECL_THIS;
  if ( _this == NULL )
    return false;
  return _this->is_focused();
}

size_t pyscv_count(PyObject *py_this)
{
  DECL_THIS;
  return _this == NULL ? 0 : _this->count();
}

bool pyscv_show(PyObject *py_this)
{
  DECL_THIS;
  return _this == NULL ? false : _this->show();
}

void pyscv_close(PyObject *py_this)
{
  DECL_THIS;
  if ( _this != NULL )
    _this->close();
}

bool pyscv_jumpto(PyObject *py_this, size_t ln, int x, int y)
{
  DECL_THIS;
  if ( _this == NULL )
    return false;
  return _this->jumpto(ln, x, y);
}

// Returns the line tuple
PyObject *pyscv_get_line(PyObject *py_this, size_t nline)
{
  DECL_THIS;
  if ( _this == NULL )
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    Py_RETURN_NONE;
  }
  return _this->get_line(nline);
}

//--------------------------------------------------------------------------
// Gets the position and returns a tuple (lineno, x, y)
PyObject *pyscv_get_pos(PyObject *py_this, bool mouse)
{
  DECL_THIS;
  if ( _this == NULL )
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    Py_RETURN_NONE;
  }
  return _this->get_pos(mouse);
}

//--------------------------------------------------------------------------
PyObject *pyscv_clear_lines(PyObject *py_this)
{
  DECL_THIS;
  if ( _this != NULL )
    _this->clear();
  PYW_GIL_CHECK_LOCKED_SCOPE();
  Py_RETURN_NONE;
}

//--------------------------------------------------------------------------
// Adds a line tuple
bool pyscv_add_line(PyObject *py_this, PyObject *py_sl)
{
  DECL_THIS;
  return _this == NULL ? false : _this->add_line(py_sl);
}

//--------------------------------------------------------------------------
bool pyscv_insert_line(PyObject *py_this, size_t nline, PyObject *py_sl)
{
  DECL_THIS;
  return _this == NULL ? false : _this->insert_line(nline, py_sl);
}

//--------------------------------------------------------------------------
bool pyscv_patch_line(PyObject *py_this, size_t nline, size_t offs, int value)
{
  DECL_THIS;
  return _this == NULL ? false : _this->patch_line(nline, offs, value);
}

//--------------------------------------------------------------------------
bool pyscv_del_line(PyObject *py_this, size_t nline)
{
  DECL_THIS;
  return _this == NULL ? false : _this->del_line(nline);
}

//--------------------------------------------------------------------------
PyObject *pyscv_get_selection(PyObject *py_this)
{
  DECL_THIS;
  if ( _this == NULL )
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    Py_RETURN_NONE;
  }
  return _this->py_get_selection();
}

//--------------------------------------------------------------------------
PyObject *pyscv_get_current_word(PyObject *py_this, bool mouse)
{
  DECL_THIS;
  PYW_GIL_CHECK_LOCKED_SCOPE();
  if ( _this != NULL )
  {
    qstring word;
    if ( _this->get_current_word(mouse, word) )
      return PyString_FromString(word.c_str());
  }
  Py_RETURN_NONE;
}

//--------------------------------------------------------------------------
// Edits an existing line
bool pyscv_edit_line(PyObject *py_this, size_t nline, PyObject *py_sl)
{
  DECL_THIS;
  return _this == NULL ? false : _this->edit_line(nline, py_sl);
}

//-------------------------------------------------------------------------
TWidget *pyscv_get_widget(PyObject *py_this)
{
  DECL_THIS;
  return _this == NULL ? NULL : _this->get_widget();
}


#undef DECL_THIS
//</inline(py_kernwin_custview)>
//---------------------------------------------------------------------------
#endif // __PYWRAPS_CUSTVIEWER__
