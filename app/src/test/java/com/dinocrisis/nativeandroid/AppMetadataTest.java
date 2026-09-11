package com.dinocrisis.nativeandroid;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class AppMetadataTest {
    @Test
    public void exposesAlphaReleaseMetadata() {
        assertEquals("v0.1.2-alpha", AppMetadata.VERSION_NAME);
        assertEquals("Dino Crisis Native Alpha", AppMetadata.APP_LABEL);
        assertTrue(AppMetadata.PRIMARY_ABI.startsWith("arm64"));
    }
}
