#ifndef __PY_GRAPH__
#define __PY_GRAPH__

//<code(py_graph)>
class py_graph_t : public py_customidamemo_t
{
  typedef py_customidamemo_t inherited;

protected:
  void collect_class_callbacks_ids(pycim_callbacks_ids_t *out);

private:
  enum
  {
    GRCODE_HAVE_HINT             = 0x00010000,
    GRCODE_HAVE_EDGE_HINT        = 0x00020000,
    GRCODE_HAVE_CLICKED          = 0x00040000,
    GRCODE_HAVE_DBL_CLICKED      = 0x00080000,
    GRCODE_HAVE_GOTFOCUS         = 0x00100000,
    GRCODE_HAVE_LOSTFOCUS        = 0x00200000,
    GRCODE_HAVE_CHANGED_CURRENT  = 0x00400000,
    GRCODE_HAVE_CREATING_GROUP   = 0x00800000,
    GRCODE_HAVE_DELETING_GROUP   = 0x01000000,
    GRCODE_HAVE_GROUP_VISIBILITY = 0x02000000,
  };
  struct nodetext_cache_t
  {
    qstring text;
    bgcolor_t bgcolor;
    nodetext_cache_t(const nodetext_cache_t &rhs): text(rhs.text), bgcolor(rhs.bgcolor) {}
    nodetext_cache_t(const char *t, bgcolor_t c): text(t), bgcolor(c) {}
    nodetext_cache_t() {}
  };

  class nodetext_cache_map_t: public std::map<int, nodetext_cache_t>
  {
  public:
    nodetext_cache_t *get(int node_id)
    {
      iterator it = find(node_id);
      if ( it == end() )
        return NULL;
      return &it->second;
    }
    nodetext_cache_t *add(const int node_id, const char *text, bgcolor_t bgcolor = DEFCOLOR)
    {
      return &(insert(std::make_pair(node_id, nodetext_cache_t(text, bgcolor))).first->second);
    }
  };

  bool refresh_needed;
  nodetext_cache_map_t node_cache;

  // instance callback
  ssize_t gr_callback(int code, va_list va);

  // static callback
  static ssize_t idaapi s_callback(void *obj, int code, va_list va)
  {
    // don't perform sanity check for 'grcode_destroyed', since if we called
    // Close() on this object, it'll have been marked for later deletion in the
    // UI, and thus when we end up here, the view has already been destroyed.
    bool found = pycim_lookup_info.find_by_py_view(NULL, (py_graph_t *) obj);
    QASSERT(30453, found || code == grcode_destroyed);
    if ( found )
    {
      PYW_GIL_GET;
      return ((py_graph_t *)obj)->gr_callback(code, va);
    }
    else
    {
      return 0;
    }
  }

  // Refresh user-defined graph node number and edges
  // It calls Python method and expects that the user already filled
  // the nodes and edges. The nodes and edges are retrieved and passed to IDA
  void on_user_refresh(mutable_graph_t *g);

  // Retrieves the text for user-defined graph node
  // It expects either a string or a tuple (string, bgcolor)
  bool on_user_text(mutable_graph_t * /*g*/, int node, const char **str, bgcolor_t *bg_color);

  // Retrieves the hint for the user-defined graph
  // Calls Python and expects a string or None
  int on_hint(char **hint, int node);
  int on_edge_hint(char **hint, int src, int dest);
  int _on_hint_epilog(char **hint, ref_t result);

  // graph is being destroyed
  void on_graph_destroyed(mutable_graph_t * /*g*/ = NULL)
  {
    refresh_needed = true;
    node_cache.clear();
  }

  // graph is being clicked
  int on_clicked(
        graph_viewer_t * /*view*/,
        selection_item_t * /*item1*/,
        graph_item_t *item2)
  {
    // in:  graph_viewer_t *view
    //      selection_item_t *current_item1
    //      graph_item_t *current_item2
    // out: 0-ok, 1-ignore click
    // this callback allows you to ignore some clicks.
    // it occurs too early, internal graph variables are not updated yet
    // current_item1, current_item2 point to the same thing
    // item2 has more information.
    // see also: kernwin.hpp, custom_viewer_click_t
    if ( item2->n == -1 )
      return 1;

    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_CLICK,
                    "i",
                    item2->n));
    PyW_ShowCbErr(S_ON_CLICK);
    return result == NULL || !PyObject_IsTrue(result.o);
  }

  // a graph node has been double clicked
  int on_dblclicked(graph_viewer_t * /*view*/, selection_item_t *item)
  {
    // in:  graph_viewer_t *view
    //      selection_item_t *current_item
    // out: 0-ok, 1-ignore click
    //graph_viewer_t *v   = va_arg(va, graph_viewer_t *);
    //selection_item_t *s = va_arg(va, selection_item_t *);
    if ( item == NULL || !item->is_node )
      return 1;

    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_DBL_CLICK,
                    "i",
                    item->node));
    PyW_ShowCbErr(S_ON_DBL_CLICK);
    return result == NULL || !PyObject_IsTrue(result.o);
  }

  // a graph viewer got focus
  void on_gotfocus(graph_viewer_t * /*view*/)
  {
    if ( self.o == NULL )
      return;

    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_ACTIVATE,
                    NULL));
    PyW_ShowCbErr(S_ON_ACTIVATE);
  }

  // a graph viewer lost focus
  void on_lostfocus(graph_viewer_t * /*view*/)
  {
    if ( self.o == NULL )
      return;

    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_DEACTIVATE,
                    NULL));
    PyW_ShowCbErr(S_ON_DEACTIVATE);
  }

  // a new graph node became the current node
  int on_changed_current(graph_viewer_t * /*view*/, int curnode)
  {
    // in:  graph_viewer_t *view
    //      int curnode
    // out: 0-ok, 1-forbid to change the current node
    if ( curnode < 0 )
      return 0;

    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_SELECT,
                    "i",
                    curnode));
    PyW_ShowCbErr(S_ON_SELECT);
    return !(result != NULL && PyObject_IsTrue(result.o));
  }

  // a group is being created
  int on_creating_group(mutable_graph_t *my_g, intvec_t *my_nodes)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    newref_t py_nodes(PyList_New(my_nodes->size()));
    int i;
    intvec_t::const_iterator p;
    for ( i = 0, p=my_nodes->begin(); p != my_nodes->end(); ++p, ++i )
      PyList_SetItem(py_nodes.o, i, PyInt_FromLong(*p));
    newref_t py_result(
            PyObject_CallMethod(
                    self.o,
                    (char *)S_ON_CREATING_GROUP,
                    "O",
                    py_nodes.o));
    PyW_ShowCbErr(S_ON_CREATING_GROUP);
    return (py_result == NULL || !PyInt_Check(py_result.o)) ? 1 : PyInt_AsLong(py_result.o);
  }

  // a group is being deleted
  int on_deleting_group(mutable_graph_t * /*g*/, int old_group)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    // TODO
    return 0;
  }

  // a group is being collapsed/uncollapsed
  int on_group_visibility(mutable_graph_t * /*g*/, int group, bool expand)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();
    // TODO
    return 0;
  }


  void show()
  {
    TWidget *view;
    if ( pycim_lookup_info.find_by_py_view(&view, this) )
      display_widget(view, WOPN_TAB);
  }

  void jump_to_node(int nid)
  {
    ref_t nodes(PyW_TryGetAttrString(self.o, S_M_NODES));
    if ( nid >= PyList_Size(nodes.o) )
      return;

    viewer_center_on(view, nid);
    int x, y;

    // will return a place only when a node was previously selected
    place_t *old_pl = get_custom_viewer_place(view, false, &x, &y);
    if ( old_pl != NULL )
    {
      user_graph_place_t *new_pl = (user_graph_place_t *) old_pl->clone();
      new_pl->node = nid;
      jumpto(view, new_pl, x, y);
      delete new_pl;
    }
  }

  virtual void refresh()
  {
    refresh_needed = true;
    inherited::refresh();
  }

  int initialize(PyObject *self, const char *title)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();

    if ( !collect_pyobject_callbacks(self) )
      return -1;

    TWidget *widget = find_widget(title);
    if ( widget == NULL ) // create new widget
    {
      lookup_entry_t &e = pycim_lookup_info.new_entry(this);
      // get a unique graph id
      netnode id;
      char grnode[MAXSTR];
      qsnprintf(grnode, sizeof(grnode), "$ pygraph %s", title);
      id.create(grnode);
      // pre-bind 'self', so that 'on_user_refresh()' can complete
      this->self = borref_t(self);
      graph_viewer_t *pview = create_graph_viewer(title, id, s_callback, this, 0);
      this->self = ref_t();
      display_widget(pview, WOPN_TAB);
      newref_t ret(PyObject_CallMethod(self, "hook", NULL));
      if ( pview != NULL )
        viewer_fit_window(pview);
      bind(self, pview);
      pycim_lookup_info.commit(e, view);
    }
    else
    {
      show();
    }

    viewer_fit_window(view);
    return 0;
  }

public:
  py_graph_t()
  {
    // form = NULL;
    refresh_needed = true;
  }

  static void SelectNode(PyObject *self, int nid)
  {
    if ( nid < 0 )
      return;

    py_graph_t *_this = view_extract_this<py_graph_t>(self);
    if ( _this == NULL || !pycim_lookup_info.find_by_py_view(NULL, _this) )
      return;

    _this->jump_to_node(nid);
  }

  static py_graph_t *Close(PyObject *self)
  {
    TWidget *view;
    py_graph_t *_this = view_extract_this<py_graph_t>(self);
    if ( _this == NULL || !pycim_lookup_info.find_by_py_view(&view, _this) )
      return NULL;
    newref_t ret(PyObject_CallMethod(self, "unhook", NULL));
    close_widget(view, WCLS_CLOSE_LATER);
    return _this;
  }

  static py_graph_t *Show(PyObject *self)
  {
    PYW_GIL_CHECK_LOCKED_SCOPE();

    py_graph_t *py_graph = view_extract_this<py_graph_t>(self);

    // New instance?
    if ( py_graph == NULL )
    {
      qstring title;
      if ( !PyW_GetStringAttr(self, S_M_TITLE, &title) )
        return NULL;

      // Form already created? try to get associated py_graph instance
      // so that we reuse it
      TWidget *existing = find_widget(title.c_str());
      if ( existing != NULL )
        pycim_lookup_info.find_by_view((py_customidamemo_t**) &py_graph, existing);

      if ( py_graph == NULL )
      {
        py_graph = new py_graph_t();
      }
      else
      {
        // unbind so we are rebound
        py_graph->unbind(false);
        py_graph->refresh_needed = true;
      }
      if ( py_graph->initialize(self, title.c_str()) < 0 )
      {
        delete py_graph;
        py_graph = NULL;
      }
    }
    else
    {
      py_graph->show();
    }
    return py_graph;
  }
};

//-------------------------------------------------------------------------
void py_graph_t::collect_class_callbacks_ids(pycim_callbacks_ids_t *out)
{
  inherited::collect_class_callbacks_ids(out);
  out->add(S_ON_REFRESH, 0);
  out->add(S_ON_GETTEXT, 0);
  out->add(S_M_EDGES, -1);
  out->add(S_M_NODES, -1);
  out->add(S_ON_HINT, GRCODE_HAVE_HINT);
  out->add(S_ON_EDGE_HINT, GRCODE_HAVE_EDGE_HINT);
  out->add(S_ON_CLICK, GRCODE_HAVE_CLICKED);
  out->add(S_ON_DBL_CLICK, GRCODE_HAVE_DBL_CLICKED);
  out->add(S_ON_SELECT, GRCODE_HAVE_CHANGED_CURRENT);
  out->add(S_ON_ACTIVATE, GRCODE_HAVE_GOTFOCUS);
  out->add(S_ON_DEACTIVATE, GRCODE_HAVE_LOSTFOCUS);
  out->add(S_ON_CREATING_GROUP, GRCODE_HAVE_CREATING_GROUP);
  out->add(S_ON_DELETING_GROUP, GRCODE_HAVE_DELETING_GROUP);
  out->add(S_ON_GROUP_VISIBILITY, GRCODE_HAVE_GROUP_VISIBILITY);
}

//-------------------------------------------------------------------------
void py_graph_t::on_user_refresh(mutable_graph_t *g)
{
  if ( !refresh_needed )
    return;

  // Check return value to OnRefresh() call
  PYW_GIL_CHECK_LOCKED_SCOPE();
  newref_t ret(PyObject_CallMethod(self.o, (char *)S_ON_REFRESH, NULL));
  PyW_ShowCbErr(S_ON_REFRESH);
  if ( ret != NULL && PyObject_IsTrue(ret.o) )
  {
    // Refer to the nodes
    ref_t nodes(PyW_TryGetAttrString(self.o, S_M_NODES));
    if ( ret != NULL && PyList_Check(nodes.o) )
    {
      // Refer to the edges
      ref_t edges(PyW_TryGetAttrString(self.o, S_M_EDGES));
      if ( ret != NULL && PyList_Check(edges.o) )
      {
        // Resize the nodes
        int max_nodes = abs(int(PyList_Size(nodes.o)));
        g->clear();
        g->resize(max_nodes);

        // Mark that we refreshed already
        refresh_needed = false;

        // Clear cached nodes
        node_cache.clear();

        // Get the edges
        for ( int i=(int)PyList_Size(edges.o)-1; i >= 0; i-- )
        {
          // Each list item is a sequence (id1, id2)
          borref_t item(PyList_GetItem(edges.o, i));
          if ( !PySequence_Check(item.o) )
            continue;

          // Get and validate each of the two elements in the sequence
          int edge_ids[2];
          int j;
          for ( j=0; j < qnumber(edge_ids); j++ )
          {
            newref_t id(PySequence_GetItem(item.o, j));
            if ( id == NULL || !PyInt_Check(id.o) )
              break;
            int v = int(PyInt_AS_LONG(id.o));
            if ( v > max_nodes )
              break;
            edge_ids[j] = v;
          }

          // Incomplete?
          if ( j != qnumber(edge_ids) )
            break;

          // Add the edge
          g->add_edge(edge_ids[0], edge_ids[1], NULL);
        }
      }
    }
  }
}

//-------------------------------------------------------------------------
bool py_graph_t::on_user_text(mutable_graph_t * /*g*/, int node, const char **str, bgcolor_t *bg_color)
{
  // If already cached then return the value
  nodetext_cache_t *c = node_cache.get(node);
  if ( c != NULL )
  {
    *str = c->text.c_str();
    if ( bg_color != NULL )
      *bg_color = c->bgcolor;
    return true;
  }

  // Not cached, call Python
  PYW_GIL_CHECK_LOCKED_SCOPE();
  newref_t result(PyObject_CallMethod(self.o, (char *)S_ON_GETTEXT, "i", node));
  PyW_ShowCbErr(S_ON_GETTEXT);
  if ( result == NULL )
    return false;

  bgcolor_t cl = bg_color == NULL ? DEFCOLOR : *bg_color;
  const char *s;

  // User returned a string?
  if ( PyString_Check(result.o) )
  {
    s = PyString_AsString(result.o);
    if ( s == NULL )
      s = "";
    c = node_cache.add(node, s, cl);
  }
  // User returned a sequence of text and bgcolor
  else if ( PySequence_Check(result.o) && PySequence_Size(result.o) == 2 )
  {
    newref_t py_str(PySequence_GetItem(result.o, 0));
    newref_t py_color(PySequence_GetItem(result.o, 1));

    if ( py_str == NULL || !PyString_Check(py_str.o) || (s = PyString_AsString(py_str.o)) == NULL )
      s = "";
    if ( py_color != NULL && PyNumber_Check(py_color.o) )
      cl = bgcolor_t(PyLong_AsUnsignedLong(py_color.o));

    c = node_cache.add(node, s, cl);
  }

  *str = c->text.c_str();
  if ( bg_color != NULL )
    *bg_color = c->bgcolor;

  return true;
}

//-------------------------------------------------------------------------
int py_graph_t::on_hint(char **hint, int node)
{
  PYW_GIL_CHECK_LOCKED_SCOPE();
  newref_t result(PyObject_CallMethod(self.o, (char *)S_ON_HINT, "i", node));
  PyW_ShowCbErr(S_ON_HINT);
  return _on_hint_epilog(hint, result);
}

//-------------------------------------------------------------------------
int py_graph_t::on_edge_hint(char **hint, int src, int dest)
{
  PYW_GIL_CHECK_LOCKED_SCOPE();
  newref_t result(PyObject_CallMethod(self.o, (char *)S_ON_EDGE_HINT, "ii", src, dest));
  PyW_ShowCbErr(S_ON_EDGE_HINT);
  return _on_hint_epilog(hint, result);
}

//-------------------------------------------------------------------------
int py_graph_t::_on_hint_epilog(char **hint, ref_t result)
{
  // 'hint' must be allocated by qalloc() or qstrdup()
  // out: 0-use default hint, 1-use proposed hint
  bool ok = result != NULL && PyString_Check(result.o);
  if ( ok )
    *hint = qstrdup(PyString_AsString(result.o));
  return ok;
}

//-------------------------------------------------------------------------
ssize_t py_graph_t::gr_callback(int code, va_list va)
{
  int ret;
  switch ( code )
  {
    //
    case grcode_user_text:
      {
        mutable_graph_t *g  = va_arg(va, mutable_graph_t *);
        int node            = va_arg(va, int);
        const char **result = va_arg(va, const char **);
        bgcolor_t *bgcolor  = va_arg(va, bgcolor_t *);
        ret = on_user_text(g, node, result, bgcolor);
        break;
      }
      //
    case grcode_destroyed:
      on_graph_destroyed(va_arg(va, mutable_graph_t *));
      ret = 0;
      break;

      //
    case grcode_clicked:
      if ( has_callback(GRCODE_HAVE_CLICKED) )
      {
        graph_viewer_t *view     = va_arg(va, graph_viewer_t *);
        selection_item_t *item = va_arg(va, selection_item_t *);
        graph_item_t *gitem    = va_arg(va, graph_item_t *);
        ret = on_clicked(view, item, gitem);
      }
      else
      {
        // Ignore the click
        ret = 1;
      }
      break;
      //
    case grcode_dblclicked:
      if ( has_callback(GRCODE_HAVE_DBL_CLICKED) )
      {
        graph_viewer_t *view     = va_arg(va, graph_viewer_t *);
        selection_item_t *item = va_arg(va, selection_item_t *);
        ret = on_dblclicked(view, item);
      }
      else
        ret = 0; // We don't want to ignore the double click, but rather
                 // fallback to the default behavior (e.g., double-clicking
                 // on an edge will to jump to the node on the other side
                 // of that edge.)
      break;
      //
    case grcode_gotfocus:
      if ( has_callback(GRCODE_HAVE_GOTFOCUS) )
        on_gotfocus(va_arg(va, graph_viewer_t *));

      ret = 0;
      break;
      //
    case grcode_lostfocus:
      if ( has_callback(GRCODE_HAVE_LOSTFOCUS) )
        on_lostfocus(va_arg(va, graph_viewer_t *));

      ret = 0;
      break;
      //
    case grcode_user_refresh:
      on_user_refresh(va_arg(va, mutable_graph_t *));

      ret = 1;
      break;
      //
    case grcode_user_hint:
      {
        mutable_graph_t *g = va_arg(va, mutable_graph_t *);
        int node = va_arg(va, int);
        int src = va_arg(va, int);
        int dest = va_arg(va, int);
        char **hint = va_arg(va, char **);
        if ( node == -1 && has_callback(GRCODE_HAVE_EDGE_HINT) )
          ret = on_edge_hint(hint, src, dest);
        else if ( node >= 0 && has_callback(GRCODE_HAVE_HINT) )
          ret = on_hint(hint, node);
        else
          ret = 0;
      }
      break;
      //
    case grcode_changed_current:
      if ( has_callback(GRCODE_HAVE_CHANGED_CURRENT) )
      {
        graph_viewer_t *view = va_arg(va, graph_viewer_t *);
        int cur_node = va_arg(va, int);
        ret = on_changed_current(view, cur_node);
      }
      else
        ret = 0; // allow selection change
      break;
      //
    case grcode_creating_group:      // a group is being created
      if ( has_callback(GRCODE_HAVE_CREATING_GROUP) )
      {
        mutable_graph_t *g = va_arg(va, mutable_graph_t*);
        intvec_t *nodes = va_arg(va, intvec_t*);
        ret = on_creating_group(g, nodes);
      }
      else
      {
        ret = 0; // Ok to create
      }
      break;
      //
    case grcode_deleting_group:      // a group is being deleted
      if ( has_callback(GRCODE_HAVE_DELETING_GROUP) )
      {
        mutable_graph_t *g = va_arg(va, mutable_graph_t*);
        int old_group = va_arg(va, int);
        ret = on_deleting_group(g, old_group);
      }
      else
      {
        ret = 0; // Ok to delete
      }
      break;
      //
    case grcode_group_visibility:    // a group is being collapsed/uncollapsed
      if ( has_callback(GRCODE_HAVE_GROUP_VISIBILITY) )
      {
        mutable_graph_t *g = va_arg(va, mutable_graph_t*);
        int group = va_arg(va, int);
        bool expand = bool(va_arg(va, int));
        ret = on_group_visibility(g, group, expand);
      }
      else
      {
        ret = 0; // Ok.
      }
      break;
      //
    default:
      ret = 0;
      break;
  }
  //grcode_changed_graph,       // new graph has been set
  //grcode_user_size,           // calculate node size for user-defined graph
  //grcode_user_title,          // render node title of a user-defined graph
  //grcode_user_draw,           // render node of a user-defined graph
  return ret;
}

//-------------------------------------------------------------------------
bool pyg_show(PyObject *self)
{
  return py_graph_t::Show(self) != NULL;
}

void pyg_close(PyObject *self)
{
  py_graph_t *pyg = py_graph_t::Close(self);
  if ( pyg != NULL )
    delete pyg;
}

void pyg_select_node(PyObject *self, int nid)
{
  py_graph_t::SelectNode(self, nid);
}
//</code(py_graph)>

//--------------------------------------------------------------------------

//<inline(py_graph)>
void pyg_close(PyObject *self);
void pyg_select_node(PyObject *self, int nid);
bool pyg_show(PyObject *self);
//</inline(py_graph)>
#endif
