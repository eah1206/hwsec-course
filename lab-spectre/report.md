## 1-1

**Given the attack plan above, how many addresses need to be flushed in the first step?**
256 addresses.

Because the secret byte is a char, it can be any value from 0-255. The kernel uses the value of the byte as an index into the shared memory, so we must flush all 256 pages.



## 2-1

**Copy your code in `run_attacker` from `attacker-part1.c` to `attacker-part2.c`. Does your Flush+Reload attack from Part 1 still work? Why or why not?**
It does not. This is because Flush+Reload requires a shared memory region since the attacker must be able to flush and then observe the exact cache lines the victim accesses. Additionally, because of the *if offset < part2_limit* check, only the first 4 bytes can be loaded non-speculatively. This is why training the branch predictor is necessary to make the attack work in part 2.



## 2-3

**In our example, the attacker tries to leak the values in the array `secret_part2`. In a real-world attack, attackers can use Spectre to leak data located in an arbitrary address in the victim's space. Explain how an attacker can achieve such leakage.**

In our lab, the kernel conveniently loads kernel_secret[offset] before the bounds check, so we can only aim at the secret array. But in a real attack,

- The attacker would find a gadget in the victim code of the form *load array[untrusted_index * stride]*

- Instead of using a valid array index, they pass *(target_address - array_base_address) / stride* as the index.

- This makes the speculative load read from *target_address - any address in the victim's space*.

- The loaded value is hten used to index into a Flush+Reload probe array, leaking it through the side channel.



## 2-4

**Experiment with how often you train the branch predictor. What is the minimum number of times you need to train the branch (i.e. `if offset < part2_limit`) to make the attack work?**

The minimum amount of training rounds was **1**. Even with just one training loop, the branch predictor retains state across kernel calls within the same run, so a single "taken" observation is enough to bias it. The kernel's explicit flush of part2_limit provides a very generous speculation window, meaning the speculative load has plenty of time to complete even with minimal predictor training.


## 3-2

**Describe the strategy you employed to extend the speculation window of the target branch in the victim.**

To extend the speculation window, we combined two approaches. Firstly, we trained the branch predictor by calling the victim many times with a valid offset, so the CPU gets used to taking the branch.
Second, we evicted from cache by writing to a large 32MB buffer before the victim call.
This forces the CPU to go all the way to DRAM to load the limit variable, giving us a longer window where the CPU is speculatively executing before it figures out the branch shouldn't have been taken.
This then helps leak the secret.


## 3-3

**Assume you are an attacker looking to exploit a new machine that has the same kernel module installed as the one we attacked in this part. What information would you need to know about this new machine to port your attack? Could it be possible to determine this infomration experimentally? Briefly describe in 5 sentences or less.**

Some of the information we would need to know is the CPU's cache timing thresholds, last level cache size (LLC), and how many branch training iterations are needed. We could determine this information experimentally from trial-and-error and by timing memory accesses and testing eviction buffer sizes.

