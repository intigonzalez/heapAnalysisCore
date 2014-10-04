package org.heapexplorer.heapanalysis;

/**
 * Created by inti on 01/10/14.
 */
public class PrincipalClassDetailsUsage {
    public final String resourceId;
    public final long totalConsumption;
    public final ClassDetailsUsage[] detailsUsages;

    public PrincipalClassDetailsUsage(String resourceId, long totalConsumption, ClassDetailsUsage[] detailsUsages) {
        this.resourceId = resourceId;
        this.totalConsumption = totalConsumption;
        this.detailsUsages = detailsUsages;
    }
}
