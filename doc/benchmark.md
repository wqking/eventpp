# Benchmarks

## EventQueue enqueue and process -- single threading

<table>
<tr>
	<th>Iterations</th>
	<th>Queue size</th>
	<th>Event count</th>
	<th>Event Types</th>
	<th>Listener count</th>
	<th>Time of single threading</th>
	<th>Time of multi threading</th>
</tr>
<tr>
	<td>100k</td>
	<td>100</td>
	<td>10M</td>
	<td>100</td>
	<td>100</td>
	<td>401</td>
	<td>1146</td>
</tr>
<tr>
	<td>100k</td>
	<td>1000</td>
	<td>100M</td>
	<td>100</td>
	<td>100</td>
	<td>4012</td>
	<td>11467</td>
</tr>
<tr>
	<td>100k</td>
	<td>1000</td>
	<td>100M</td>
	<td>1000</td>
	<td>1000</td>
	<td>4102</td>
	<td>11600</td>
</tr>
<table>

Given `eventpp::EventQueue<int, void (int), Policies>`, which `Policies` is either single threading or multi threading, the benchmark adds `Listener count` listeners to the queue, each listener is an empty lambda. Then the benchmark starts timing. It loops `Iterations` times. In each loop, the benchmark puts `Queue size` events, then process the event queue.  
There are `Event types` kinds of event type. `Event count` is `Iterations * Queue size`.  
The EventQueue is processed in one thread. The Single/Multi threading in the table means the policies used.

## EventQueue enqueue and process -- multiple threading

<table>
<tr>
	<th>Enqueue threads</th>
	<th>Process threads</th>
	<th>Event count</th>
	<th>Event Types</th>
	<th>Listener count</th>
	<th>Time</th>
</tr>
<tr>
	<td>1</td>
	<td>1</td>
	<td>10M</td>
	<td>100</td>
	<td>100</td>
	<td>2387</td>
</tr>
<tr>
	<td>1</td>
	<td>1</td>
	<td>100M</td>
	<td>100</td>
	<td>100</td>
	<td>23656</td>
</tr>
<tr>
	<td>1</td>
	<td>3</td>
	<td>10M</td>
	<td>100</td>
	<td>100</td>
	<td>3755</td>
</tr>
<tr>
	<td>1</td>
	<td>3</td>
	<td>100M</td>
	<td>100</td>
	<td>100</td>
	<td>37983</td>
</tr>
<tr>
	<td>2</td>
	<td>2</td>
	<td>10M</td>
	<td>100</td>
	<td>100</td>
	<td>4323</td>
</tr>
<tr>
	<td>2</td>
	<td>2</td>
	<td>100M</td>
	<td>100</td>
	<td>100</td>
	<td>42263</td>
</tr>
</table>

There are `Enqueue threads` threads enqueuing events to the queue, and `Process threads` threads processing the events. The total event count is `Event count`.  
The multi threading version shows slower than previous single threading version, since the mutex locks cost time.

## CallbackList invoking VS native function invoking

Hardware: Intel(R) Xeon(R) CPU E3-1225 V2 @ 3.20GHz  
Software: Windows 10, MSVC 2017, MinGW GCC 7.2.0  
Iterations: 100,000,000  
Time unit: milliseconds

<table>
<tr>
	<th>Function</th>
	<th>Compiler</th>
	<th>Native invoking</th>
	<th>CallbackList single threading</th>
	<th>CallbackList multi threading</th>
</tr>

<tr>
	<td rowspan="2">Inline global function</td>
	<td>MSVC 2017</td>
	<td>217</td>
	<td>1501</td>
	<td>6921</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>187</td>
	<td>1489</td>
	<td>4463</td>
</tr>

<tr>
	<td rowspan="2">Non-inline global function</td>
	<td>MSVC 2017</td>
	<td>241</td>
	<td>1526</td>
	<td>6544</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>233</td>
	<td>1488</td>
	<td>4787</td>
</tr>

<tr>
	<td rowspan="2">Function object</td>
	<td>MSVC 2017</td>
	<td>194</td>
	<td>1498</td>
	<td>6433</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>212</td>
	<td>1485</td>
	<td>4951</td>
</tr>

<tr>
	<td rowspan="2">Member virtual function</td>
	<td>MSVC 2017</td>
	<td>207</td>
	<td>1533</td>
	<td>6558</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>212</td>
	<td>1485</td>
	<td>4489</td>
</tr>

<tr>
	<td rowspan="2">Member non-virtual function</td>
	<td>MSVC 2017</td>
	<td>214</td>
	<td>1533</td>
	<td>6390</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>211</td>
	<td>1486</td>
	<td>4872</td>
</tr>

<tr>
	<td rowspan="2">Member non-inline virtual function</td>
	<td>MSVC 2017</td>
	<td>206</td>
	<td>1522</td>
	<td>6578</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>182</td>
	<td>1666</td>
	<td>4593</td>
</tr>

<tr>
	<td rowspan="2">Member non-inline non-virtual function</td>
	<td>MSVC 2017</td>
	<td>206</td>
	<td>1491</td>
	<td>6992</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>205</td>
	<td>1486</td>
	<td>4490</td>
</tr>

<tr>
	<td rowspan="2">All functions</td>
	<td>MSVC 2017</td>
	<td>1374</td>
	<td>10951</td>
	<td>29973</td>
</tr>
<tr>
	<td>GCC 7.2</td>
	<td>1223</td>
	<td>9770</td>
	<td>22958</td>
</tr>

</table>

Testing functions  
```c++
#if defined(_MSC_VER)
#define NON_INLINE __declspec(noinline)
#else
// gcc
#define NON_INLINE __attribute__((noinline))
#endif

volatile int globalValue = 0;

void globalFunction(int a, const int b)
{
	globalValue += a + b;
}

NON_INLINE void nonInlineGlobalFunction(int a, const int b)
{
	globalValue += a + b;
}

struct FunctionObject
{
	void operator() (int a, const int b)
	{
		globalValue += a + b;
	}

	virtual void virFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	void nonVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	NON_INLINE virtual void nonInlineVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	NON_INLINE void nonInlineNonVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}
};

#undef NON_INLINE
```
