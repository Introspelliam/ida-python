from __future__ import print_function
import idaapi

# -----------------------------------------------------------------------
# Using raw IDAAPI
def raw_main(p=True):
    f = idaapi.get_func(here())
    if not f:
        return

    q = idaapi.qflow_chart_t("The title", f, 0, 0, idaapi.FC_PREDS)
    for n in xrange(0, q.size()):
        b = q[n]
        if p:
            print("%x - %x [%d]:" % (b.start_ea, b.end_ea, n))

        for ns in xrange(0, q.nsucc(n)):
            if p:
                print("SUCC:  %d->%d" % (n, q.succ(n, ns)))

        for ns in xrange(0, q.npred(n)):
            if p:
                print("PRED:  %d->%d" % (n, q.pred(n, ns)))

# -----------------------------------------------------------------------
# Using the class
def cls_main(p=True):
    f = idaapi.FlowChart(idaapi.get_func(here()))
    for block in f:
        if p:
            print("%x - %x [%d]:" % (block.start_ea, block.end_ea, block.id))
        for succ_block in block.succs():
            if p:
                print("  %x - %x [%d]:" % (succ_block.start_ea, succ_block.end_ea, succ_block.id))

        for pred_block in block.preds():
            if p:
                print("  %x - %x [%d]:" % (pred_block.start_ea, pred_block.end_ea, pred_block.id))

q = None
f = None
raw_main(False)
cls_main(True)

