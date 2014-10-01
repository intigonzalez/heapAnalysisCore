package org.heapexplorer.heapanalysis;

/**
 * Created by inti on 01/10/14.
 */
public class PrincipalClassDetailsUsage {
    public final String resourceId;
    public final ClassDetailsUsage[] detailsUsages;

    public PrincipalClassDetailsUsage(String resourceId, ClassDetailsUsage[] detailsUsages) {
        this.resourceId = resourceId;
        this.detailsUsages = detailsUsages;
    }
}
