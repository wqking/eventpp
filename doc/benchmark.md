# Benchmarks

## CallbackList invoking VS native function invoking

Hardware: Intel(R) Xeon(R) CPU E3-1225 V2 @ 3.20GHz  
Software: Windows 10, MSVC 2017, MinGW GCC 7.2.0  
Iterations: 100,100,100  
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
