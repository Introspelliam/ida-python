#<pycode(py_kernwin_viewhooks)>
# -----------------------------------------------------------------------
#                           CustomIDAMemo
# -----------------------------------------------------------------------
class CustomIDAMemo(View_Hooks):
    def __init__(self):
        View_Hooks.__init__(self)

    def _graph_item_tuple(self, ve):
        item = None
        if ve.rtype in [TCCRT_GRAPH, TCCRT_PROXIMITY]:
            item = ve.location.item
        if item is not None:
            if item.is_node:
                return (item.node,)
            else:
                return (item.elp.e.src, item.elp.e.dst)
        else:
            return ()

    @staticmethod
    def _dummy_cb(*args):
        pass

    def _get_cb(self, view, cb_name):
        cb = CustomIDAMemo._dummy_cb
        if view == self.GetWidget():
            cb = getattr(self, cb_name, cb)
        return cb

    def _get_cb_arity(self, cb):
        from inspect import getargspec
        return len(getargspec(cb).args)

    def view_activated(self, view):
        return self._get_cb(view, "OnViewActivated")()

    def view_deactivated(self, view):
        return self._get_cb(view, "OnViewDeactivated")()

    def view_keydown(self, view, key, state):
        return self._get_cb(view, "OnViewKeydown")(key, state)

    def view_click(self, view, ve):
        cb = self._get_cb(view, "OnViewClick")
        if cb != CustomIDAMemo._dummy_cb:
            arity = self._get_cb_arity(cb)
            args = [ve.x, ve.y, ve.state]
            if arity >= 5:
                args.append(ve.button)
                if arity >= 6:
                    args.append(ve.renderer_pos)
            return cb(*tuple(args))

    def view_dblclick(self, view, ve):
        cb = self._get_cb(view, "OnViewDblclick")
        if cb != CustomIDAMemo._dummy_cb:
            arity = self._get_cb_arity(cb)
            args = [ve.x, ve.y, ve.state]
            if arity >= 5:
                args.append(ve.renderer_pos)
            return cb(*tuple(args))

    def view_curpos(self, view, *args):
        return self._get_cb(view, "OnViewCurpos")()

    def view_close(self, view, *args):
        rc = self._get_cb(view, "OnClose")()
        if view == self.GetWidget():
            ida_idaapi.pycim_view_close(self)
        return rc

    def view_switched(self, view, rt):
        return self._get_cb(view, "OnViewSwitched")(rt)

    def view_mouse_over(self, view, ve):
        cb = self._get_cb(view, "OnViewMouseOver")
        if cb != CustomIDAMemo._dummy_cb:
            arity = self._get_cb_arity(cb)
            gitpl = self._graph_item_tuple(ve)
            args = [ve.x, ve.y, ve.state, len(gitpl), gitpl]
            if arity >= 7:
                args.append(ve.renderer_pos)
            return cb(*tuple(args))

    def view_loc_changed(self, view, now, was):
        return self._get_cb(view, "OnViewLocationChanged")(now, was)

    def view_mouse_moved(self, view, ve):
        cb = self._get_cb(view, "OnViewMouseMoved")
        if cb != CustomIDAMemo._dummy_cb:
            gitpl = self._graph_item_tuple(ve)
            return cb(ve.x, ve.y, ve.state, len(gitpl), gitpl, ve.renderer_pos)

    # End of hooks->wrapper trampolines


    def Refresh(self):
        """
        Refreshes the view. This causes the OnRefresh() to be called
        """
        ida_idaapi.pygc_refresh(self)

    def GetCurrentRendererType(self):
        return ida_idaapi.pygc_get_current_renderer_type(self)

    def SetCurrentRendererType(self, rtype):
        """
        Set the current view's renderer.

        @param rtype: The renderer type. Should be one of the idaapi.TCCRT_* values.
        """
        ida_idaapi.pygc_set_current_renderer_type(self, rtype)

    def SetNodeInfo(self, node_index, node_info, flags):
        """
        Set the properties for the given node.

        Example usage (set second nodes's bg color to red):
          inst = ...
          p = idaapi.node_info_t()
          p.bg_color = 0x00ff0000
          inst.SetNodeInfo(1, p, idaapi.NIF_BG_COLOR)

        @param node_index: The node index.
        @param node_info: An idaapi.node_info_t instance.
        @param flags: An OR'ed value of NIF_* values.
        """
        ida_idaapi.pygc_set_node_info(self, node_index, node_info, flags)

    def SetNodesInfos(self, values):
        """
        Set the properties for the given nodes.

        Example usage (set first three nodes's bg color to purple):
          inst = ...
          p = idaapi.node_info_t()
          p.bg_color = 0x00ff00ff
          inst.SetNodesInfos({0 : p, 1 : p, 2 : p})

        @param values: A dictionary of 'int -> node_info_t' objects.
        """
        ida_idaapi.pygc_set_nodes_infos(self, values)

    def GetNodeInfo(self, node):
        """
        Get the properties for the given node.

        @param node: The index of the node.
        @return: A tuple (bg_color, frame_color, ea, text), or None.
        """
        return ida_idaapi.pygc_get_node_info(self, node)

    def DelNodesInfos(self, *nodes):
        """
        Delete the properties for the given node(s).

        @param nodes: A list of node IDs
        """
        return ida_idaapi.pygc_del_nodes_infos(self, nodes)

    def CreateGroups(self, groups_infos):
        """
        Send a request to modify the graph by creating a
        (set of) group(s), and perform an animation.

        Each object in the 'groups_infos' list must be of the format:
        {
          "nodes" : [<int>, <int>, <int>, ...] # The list of nodes to group
          "text" : <string>                    # The synthetic text for that group
        }

        @param groups_infos: A list of objects that describe those groups.
        @return: A [<int>, <int>, ...] list of group nodes, or None (failure).
        """
        return ida_idaapi.pygc_create_groups(self, groups_infos)

    def DeleteGroups(self, groups, new_current = -1):
        """
        Send a request to delete the specified groups in the graph,
        and perform an animation.

        @param groups: A list of group node numbers.
        @param new_current: A node to focus on after the groups have been deleted
        @return: True on success, False otherwise.
        """
        return ida_idaapi.pygc_delete_groups(self, groups, new_current)

    def SetGroupsVisibility(self, groups, expand, new_current = -1):
        """
        Send a request to expand/collapse the specified groups in the graph,
        and perform an animation.

        @param groups: A list of group node numbers.
        @param expand: True to expand the group, False otherwise.
        @param new_current: A node to focus on after the groups have been expanded/collapsed.
        @return: True on success, False otherwise.
        """
        return ida_idaapi.pygc_set_groups_visibility(self, groups, expand, new_current)

    def GetWidget(self):
        """
        Return the TWidget underlying this view.

        @return: The TWidget underlying this view, or None.
        """
        return ida_idaapi.pycim_get_widget(self)

# ----------------------------------------------------------------------
# bw-compat/deprecated. You shouldn't rely on this in new code
import ida_idaapi
ida_idaapi.CustomIDAMemo = CustomIDAMemo

#</pycode(py_kernwin_viewhooks)>
