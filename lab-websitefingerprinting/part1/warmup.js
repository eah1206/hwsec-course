const runs = 10;

function measureOneLine() {
  const LINE_SIZE = 16; // 128/sizeof(double) Note that js treats all numbers as double
  let result = [];

  // Fill with -1 to ensure allocation
  const M = new Array(runs * LINE_SIZE).fill(-1);

  for (let i = 0; i < runs; i++) {
    const start = performance.now();
    let val = M[i * LINE_SIZE];
    const end = performance.now();

    result.push(end - start);
  }

  return result;
}

function measureNLines() {
  let result = [];
  const LINE_SIZE = 64;

  const N = 1000000;

  // Fill with -1 to ensure allocation
  const M = new Array (runs * N * LINE_SIZE).fill(-1);

  // TODO: Exercise 1-1
  // Perform 10 runs of accessing N cache lines
  for (let i=0; i < runs; i++) {
    const start = performance.now();
    // Measure the time taken to access all N cache lines in each run
    for (let j=0; j < N; j++) {
      let val = M[runs*j*LINE_SIZE];
    }
    const end = performance.now();
    result.push(end - start);
  }
  const sorted = result.toSorted();
  let median = (sorted[4]+sorted[5])/2;
  console.log(median)
  return result;
}

document.getElementById(
  "exercise1-values"
).innerText = `1 Cache Line: [${measureOneLine().join(", ")}]`;

document.getElementById(
  "exercise2-values"
).innerText = `N Cache Lines: [${measureNLines().join(", ")}]`;
