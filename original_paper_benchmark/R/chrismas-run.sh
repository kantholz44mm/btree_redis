export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH

echo "Dont forget to run /"sudo sysctl -w kernel.perf_event_paranoid=0/"!"

sleep 5s

[ ! -f "R/avg-fanout/avg-fanout.csv" ] && python3 R/avg-fanout/avg-fanout.py | shuf | parallel -j1 --timeout 600 --joblog joblog-avg-fanout -- {1} | tee R/avg-fanout/avg-fanout.csv || true
[ ! -f "R/eval-2/eval-wormhole.csv" ] && python3 R/eval-2/eval-wormhole.py | shuf | parallel -j1 --timeout 600 --joblog joblog-eval-wormhole -- {1} | tee R/eval-2/eval-wormhole.csv || true
[ ! -f "R/eval-2/re-eval-dense.csv" ] && python3 R/eval-2/re-eval-dense.py | shuf | parallel -j1 --timeout 600 --joblog joblog-re-eval-dense -- {1} | tee R/eval-2/re-eval-dense.csv || true
[ ! -f "R/eval-2/re-eval-rng8.csv" ] && python3 R/eval-2/re-eval-rng8.py | shuf | parallel -j1 --timeout 600 --joblog joblog-re-eval-rng8 -- {1} | tee R/eval-2/re-eval-rng8.csv || true
[ ! -f "R/eval-dense/dense-tasks.csv" ] && python3 R/eval-dense/dense-tasks.py | shuf | parallel -j1 --timeout 600 --joblog joblog-dense-tasks -- {1} | tee R/eval-dense/dense-tasks.csv || true
[ ! -f "R/eval-dense/partition-id-hint.csv" ] && python3 R/eval-dense/partition-id-hint.py | shuf | parallel -j1 --timeout 600 --joblog joblog-partition-id-hint -- {1} | tee R/eval-dense/partition-id-hint.csv || true
[ ! -f "R/eval-dense/task-sorted-insert.csv" ] && python3 R/eval-dense/task-sorted-insert.py | shuf | parallel -j1 --timeout 600 --joblog joblog-task-sorted-insert -- {1} | tee R/eval-dense/task-sorted-insert.csv || true
[ ! -f "R/eval-dense/var-density-e.csv" ] && python3 R/eval-dense/var-density-e.py | shuf | parallel -j1 --timeout 600 --joblog joblog-var-density-e -- {1} | tee R/eval-dense/var-density-e.csv || true
[ ! -f "R/eval-dense/var-density.csv" ] && python3 R/eval-dense/var-density.py | shuf | parallel -j1 --timeout 600 --joblog joblog-var-density -- {1} | tee R/eval-dense/var-density.csv || true
[ ! -f "R/in-mem-size/in-mem-size-lits.csv" ] && python3 R/in-mem-size/in-mem-size-lits.py | shuf | parallel -j1 --timeout 600 --joblog joblog-in-mem-size-lits -- {1} | tee R/in-mem-size/in-mem-size-lits.csv || true
[ ! -f "R/in-mem-size/in-mem-size.csv" ] && python3 R/in-mem-size/in-mem-size.py | shuf | parallel -j1 --timeout 600 --joblog joblog-in-mem-size -- {1} | tee R/in-mem-size/in-mem-size.csv || true
[ ! -f "R/space-0-payload/space-0-payload.csv" ] && python3 R/space-0-payload/space-0-payload.py | shuf | parallel -j1 --timeout 600 --joblog joblog-space-0-payload -- {1} | tee R/space-0-payload/space-0-payload.csv || true
[ ! -f "R/in-memory-skew/skew2-wh.csv" ] && python3 R/in-memory-skew/skew2-wh.py | shuf | parallel -j1 --timeout 600 --joblog joblog-skew2-wh -- {1} | tee R/in-memory-skew/skew2-wh.csv || true
[ ! -f "R/size3/vary7.csv" ] && python3 R/size3/vary7.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v7 -- {1} | tee R/size3/vary7.csv || true
[ ! -f "R/size3/vary3.csv" ] && python3 R/size3/vary3.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v3 -- {1} | tee R/size3/vary3.csv || true
[ ! -f "R/size3/vary4.csv" ] && python3 R/size3/vary4.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v4 -- {1} | tee R/size3/vary4.csv || true
[ ! -f "R/size3/leaf-size-density.csv" ] && python3 R/size3/leaf-size-density.py | shuf | parallel -j1 --timeout 600 --joblog joblog-leaf-size-density -- {1} | tee R/size3/leaf-size-density.csv || true
[ ! -f "R/size3/vary5.csv" ] && python3 R/size3/vary5.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v5 -- {1} | tee R/size3/vary5.csv || true
[ ! -f "R/size3/vary2.csv" ] && python3 R/size3/vary2.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v2 -- {1} | tee R/size3/vary2.csv || true
[ ! -f "R/size3/vary1.csv" ] && python3 R/size3/vary1.py | shuf | parallel -j1 --timeout 600 --joblog joblog-v1 -- {1} | tee R/size3/vary1.csv || true
[ ! -f "R/in-memory-skew/skew3.csv" ] && python3 R/in-memory-skew/skew3.py | shuf | parallel -j1 --timeout 600 --joblog joblog-skew3 -- {1} | tee R/in-memory-skew/skew3.csv || true
[ ! -f "R/in-memory-skew/skew.csv" ] && python3 R/in-memory-skew/skew.py | shuf | parallel -j1 --timeout 600 --joblog joblog-skew -- {1} | tee R/in-memory-skew/skew.csv || true

