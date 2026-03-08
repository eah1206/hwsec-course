// Number of sweep counts
// TODO (Exercise 2-1): Choose an appropriate value!
let P = 5;
const BUFFER_SIZE = 16*1024*1024;

// Number of elements in your trace
let K = 5 * 1000 / P; 

// Array of length K with your trace's values
let T;

// Value of performance.now() when you started recording your trace
let start;

function record() {
  // Create empty array for saving trace values
  T = new Array(K);

  // Fill array with -1 so we can be sure memory is allocated
  T.fill(-1, 0, T.length);

  // Save start timestamp
  start = performance.now();

  // TODO (Exercise 2-1): Record data for 5 seconds and save values to T.
  const LINE_SIZE = 64;
  buffer = new Array(BUFFER_SIZE/64).fill(-1);
  for(let i=0; i < K; i++) {
    let counter = 0;
    window = performance.now();
    while (performance.now() - window < P) {
      let M = buffer[i*counter*LINE_SIZE];
      counter++;
    }
    T[i]=counter;
  }


  // Once done recording, send result to main thread
  postMessage(JSON.stringify(T));
}

// DO NOT MODIFY BELOW THIS LINE -- PROVIDED BY COURSE STAFF
self.onmessage = (e) => {
  if (e.data.type === "start") {
    setTimeout(record, 0);
  }
};
