/*
 * repro_lsp_diag.c — TEMPORARY diagnostic (NOT an assertion suite).
 *
 * Dumps every CALLS edge (source label+QN, target label+QN, properties_json) for
 * a few curated-LSP fixtures, to determine whether the 89-red LSP-strategy
 * cluster shares ONE root cause (the caller_qn==enclosing_func_qn join) or many.
 * The edge source QN IS the call's enclosing_func_qn (that is how the edge was
 * built), and properties_json carries the resolution "strategy". Reading these
 * tells us: did the LSP resolution win (strategy=lsp_*) or did the generic
 * registry win (strategy=same_module/import/unknown)?
 *
 * Remove this file once the LSP cluster is diagnosed.
 */
#include "test_framework.h"
#include "repro_harness.h"
#include "repro_invariant_lib.h"

#include <stdio.h>
#include <string.h>

static void diag_dump_calls(const char *tag, const char *filename, const char *src) {
    RProj lp;
    cbm_store_t *store = rh_index(&lp, filename, src);
    printf("\n[DIAG %s] file=%s store=%s\n", tag, filename, store ? "ok" : "NULL");
    if (!store) {
        return;
    }
    cbm_edge_t *edges = NULL;
    int n = 0;
    if (cbm_store_find_edges_by_type(store, lp.project, "CALLS", &edges, &n) != CBM_STORE_OK) {
        printf("[DIAG %s] find_edges_by_type(CALLS) FAILED\n", tag);
        rh_cleanup(&lp, store);
        return;
    }
    printf("[DIAG %s] CALLS edge count = %d\n", tag, n);
    for (int i = 0; i < n; i++) {
        cbm_node_t s, t;
        const char *sl = "?", *sq = "?", *tl = "?", *tq = "?";
        if (cbm_store_find_node_by_id(store, edges[i].source_id, &s) == CBM_STORE_OK) {
            sl = s.label ? s.label : "(nolabel)";
            sq = s.qualified_name ? s.qualified_name : "(noqn)";
        }
        if (cbm_store_find_node_by_id(store, edges[i].target_id, &t) == CBM_STORE_OK) {
            tl = t.label ? t.label : "(nolabel)";
            tq = t.qualified_name ? t.qualified_name : "(noqn)";
        }
        printf("[DIAG %s] CALLS#%d  src=[%s]%s  ->  tgt=[%s]%s  props=%s\n", tag, i, sl, sq, tl,
               tq, edges[i].properties_json ? edges[i].properties_json : "(null)");
    }
    cbm_store_free_edges(edges, n);
    rh_cleanup(&lp, store);
}

void suite_repro_lsp_diag(void);
void suite_repro_lsp_diag(void) {
    /* Java lsp_type_dispatch: c.inc(1) on the object's own declared type. */
    diag_dump_calls("java_type_dispatch", "a.java",
                    "class Counter {\n"
                    "    int inc(int x) { return x + 1; }\n"
                    "    int run() {\n"
                    "        Counter c = new Counter();\n"
                    "        return c.inc(1);\n"
                    "    }\n"
                    "}\n");
    /* C# static call A.Helper(1). */
    diag_dump_calls("cs_static", "a.cs",
                    "class A {\n"
                    "    static int Helper(int x) { return x; }\n"
                    "    int Run() { return A.Helper(1); }\n"
                    "}\n");
    /* Go method dispatch c.Add(1) on own type. */
    diag_dump_calls("go_type_dispatch", "a.go",
                    "package p\n"
                    "type Calc struct{}\n"
                    "func (c Calc) Add(x int) int { return x }\n"
                    "func Run() { c := Calc{}; c.Add(1) }\n");
}
