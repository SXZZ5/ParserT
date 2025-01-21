package main

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"
)

var launched int = 0
var finished int = 0

func mutexPrintln(t any) {
	mu.Lock()
	fmt.Println(t)
	mu.Unlock()
}

func filter(str string) int {
	var res strings.Builder
	for _, r := range str {
		if r == '\r' || r == '\n' {
			continue
		}
		res.WriteRune(r)
	}

	res_str := res.String()
	ret, err := strconv.Atoi(res_str)
	if err != nil {
		fmt.Println("Error converting to int")
	}
	return ret
}

var exepath string = ""

type work struct {
	input    string
	line_no  int
	test_num int
}

var mu sync.Mutex

func worker(ctx context.Context, wg *sync.WaitGroup, goodchannel chan<- *work, wrk *work) {
	/* mu.Lock()
	fmt.Println("worker launched")
	mu.Unlock() */
	defer func() {
		finished++
		wg.Done()
		/* mu.Lock()
		fmt.Println("worker ending")
		mu.Unlock() */
	}()
	cmd := exec.CommandContext(ctx, exepath)
	stdin, _ := cmd.StdinPipe()
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	defer stdin.Close()
	defer stdout.Close()
	defer stderr.Close()
	err := cmd.Start()
	if err != nil {
		mu.Lock()
		log.Print("error in starting executable")
		log.Print(err)
		mu.Unlock()
		return
	}
	// mu.Lock()
	launched++
	// mu.Unlock()
	stdin.Write([]byte((*wrk).input))
	readstdout, _ := io.ReadAll(stdout)
	// readstderr, _ := io.ReadAll(stderr)
	err = cmd.Wait()

	if err == nil {
		goodchannel <- wrk
		mu.Lock()
		fmt.Println("i/p:", (*wrk).input)
		fmt.Println("stdout :", string(readstdout))
		// fmt.Println("stderr :", string(readstderr))
		fmt.Println(err)
		fmt.Println("=====================================")
		mu.Unlock()
	}
}

func sender(ctx context.Context, wg *sync.WaitGroup, goodchannel chan *work,
	T int, strlist []string) {
	fmt.Println("sender called")
	defer func() {
		fmt.Println("sender exiting")
	}()
	prefixes := []string{}
	for i := 1; i <= len(strlist); i++ {
		res := ""
		if i > 1 {
			res = prefixes[i-2]
		}
		res += strlist[i-1]
		prefixes = append(prefixes, res)
	}

	fmt.Println("done making prefixes")

	for i := 1; i <= T; i++ {
		tmp := strconv.Itoa(i)
		for j, l := range prefixes {
			input := tmp + "\n" + l
			one_work := work{input, j+1, i}
			/* mu.Lock()
			fmt.Println("i:", i, ",j:", j)
			mu.Unlock() */
			timeout_ctx, _ := context.WithTimeout(ctx, 1000*time.Millisecond)
			// defer can()
			(*wg).Add(1)
			go worker(timeout_ctx, wg, goodchannel, &one_work)
		}
	}
}

func waiter(ctx context.Context, goodchannel <-chan *work,
	T int, endwaiter *sync.WaitGroup, strlist []string) {
	defer endwaiter.Done()
	/* 	mu.Lock()
	   	fmt.Println("waiter arguments:")
	   	fmt.Println(T, strlist)
	   	mu.Unlock() */
	found := make([]bool, T)
	test_data := make([]*work, T)
	done_count := 0
Loop:
	for {
		select {
		case accepted := <-goodchannel:
			test_num := (*accepted).test_num
			if !found[test_num-1] {
				found[test_num-1] = true
				test_data[test_num-1] = accepted
				done_count++
			}

			if found[test_num-1] {
				new_test_data := (*accepted)
				if new_test_data.line_no < (*(test_data[test_num-1])).line_no {
					test_data[test_num-1] = accepted
				}
			}
		case <-ctx.Done():
			fmt.Println("received context ending")
			break Loop
		}
	}

	mu.Lock()
	fmt.Println("tetst_data line no values found: ")
	fmt.Println("size test_data:", len(test_data))
	for i, r := range test_data {
		fmt.Println(i, (*r).line_no)
	}
	fmt.Println("FINAL ISOLATED TESTS")
	old := 0
	for i := 1; i <= T; i++ {
		input := ""
		for j := old + 1; j <= (*test_data[i-1]).line_no; j++ {
			input += strlist[j-1]
		}
		fmt.Println(input)
		fmt.Println("========================")
		old = (*test_data[i-1]).line_no
	}
	mu.Unlock()
}

func main() {
	args := os.Args[1:]
	file, err := os.Open(args[0])
	if err != nil {
		fmt.Println("Error in opening File: ", err)
	}
	defer file.Close()

	buf := bufio.NewReader(file)
	strlist := []string{}
	for {
		line, err := buf.ReadString('\n')
		if err != nil {
			break
		}
		strlist = append(strlist, line)
	}

	str_total_tests := strlist[0]
	strlist = strlist[1:]
	T := filter(str_total_tests)
	mu.Lock()
	fmt.Println("T:", T)
	fmt.Println("len(strlist)", len(strlist))
	mu.Unlock()

	exepath, err = os.Getwd()
	if err != nil {
		fmt.Println("Error in getting current working directory")
	}
	exepath = filepath.Join(exepath, args[1])

	goodchannel := make(chan *work)
	ctx_workers, _ := context.WithCancel(context.Background())
	var wg sync.WaitGroup
	go sender(ctx_workers, &wg, goodchannel, T, strlist)

	ctx_waiter, cancelWaiter := context.WithTimeout(context.Background(), time.Second*2)
	var endwaiter sync.WaitGroup
	go waiter(ctx_waiter, goodchannel, T, &endwaiter, strlist)
	endwaiter.Add(1)

	log.Println("going to being waiting for wg waitgroup")
	wg.Wait()
	log.Println("done waiting for wg waitgroup")
	cancelWaiter()
	endwaiter.Wait()
	mutexPrintln("launched:")
	mutexPrintln(launched)
	mutexPrintln("finished:")
	mutexPrintln(finished)
}
