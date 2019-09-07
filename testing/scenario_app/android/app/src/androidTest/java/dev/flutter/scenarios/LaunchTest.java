package dev.flutter.scenarios;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.internal.runner.junit4.statement.UiThreadStatement;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

import io.flutter.embedding.engine.FlutterEngine;
import io.flutter.embedding.engine.dart.DartExecutor;

import static org.junit.Assert.fail;

@RunWith(AndroidJUnit4.class)
public class LaunchTest  {
    @Test
    public void testEngineLaunch() throws Throwable {
        Context applicationContext = InstrumentationRegistry.getTargetContext();
        // Specifically, create the engine without running FlutterMain first.
        final AtomicReference<FlutterEngine> engine = new AtomicReference<>();

        // Run the production under test on the UI thread instead of annotating the whole test
        // as @UiThreadTest because having the message handler and the CompletableFuture both being
        // on the same thread will create deadlocks.
        UiThreadStatement.runOnUiThread(
            () -> engine.set(new FlutterEngine(applicationContext))
        );
        CompletableFuture<Boolean> statusReceived = new CompletableFuture<>();

        engine.get().getDartExecutor().setMessageHandler(
            "scenario_status",
            (byteBuffer, binaryReply) -> statusReceived.complete(Boolean.TRUE)
        );

        UiThreadStatement.runOnUiThread(
            () -> engine.get().getDartExecutor().executeDartEntrypoint(DartExecutor.DartEntrypoint.createDefault())
        );

        try {
            Boolean result = statusReceived.get(10, TimeUnit.SECONDS);
            if (!result) {
                fail("expected message on scenario_status not received");
            }
        } catch (ExecutionException e) {
            fail(e.getMessage());
        } catch (InterruptedException e) {
            fail(e.getMessage());
        } catch (TimeoutException e) {
            fail("timed out waiting for engine started signal");
        }
        // If it gets to here, statusReceived is true.
    }
}
